#include <weku/chain/database.hpp>

#include <cstdint>
#include <deque>
#include <fstream>
#include <functional>

#include <fc/smart_ref_impl.hpp>
#include <fc/uint128.hpp>
#include <fc/container/deque.hpp>
#include <fc/io/fstream.hpp>

#include <weku/protocol/weku_operations.hpp>

#include <weku/chain/util/asset.hpp>
#include <weku/chain/util/reward.hpp>
#include <weku/chain/util/uint256.hpp>
#include <weku/chain/util/reward.hpp>

#include <weku/chain/block_summary_object.hpp>
#include <weku/chain/compound.hpp>
#include <weku/chain/custom_operation_interpreter.hpp>
#include <weku/chain/database_exceptions.hpp>
#include <weku/chain/db_with.hpp>
#include <weku/chain/evaluator_registry.hpp>
#include <weku/chain/global_property_object.hpp>
#include <weku/chain/index.hpp>
#include <weku/chain/weku_evaluator.hpp>
#include <weku/chain/common_objects.hpp>
#include <weku/chain/transaction_object.hpp>
#include <weku/chain/shared_db_merkle.hpp>
#include <weku/chain/operation_notification.hpp>
#include <weku/chain/operation_object.hpp>
#include <weku/chain/witness_schedule.hpp>
#include <weku/chain/blacklist_objects.hpp>
#include <weku/chain/invariant_validator.hpp>
#include <weku/chain/fund_processor.hpp>
#include <weku/chain/account_recovery_processor.hpp>
#include <weku/chain/balance_processor.hpp>
#include <weku/chain/block_applier.hpp>
#include <weku/chain/block_header_validator.hpp>
#include <weku/chain/cashout_processor.hpp>
#include <weku/chain/genesis_processor.hpp>
#include <weku/chain/gpo_processor.hpp>
#include <weku/chain/reward_processor.hpp>
#include <weku/chain/median_feed_updator.hpp>
#include <weku/chain/null_account_cleaner.hpp>
#include <weku/chain/order_processor.hpp>
#include <weku/chain/slot.hpp>
#include <weku/chain/vest_withdraw_processor.hpp>
#include <weku/chain/conversion_processor.hpp>
#include <weku/chain/hardforker.hpp>
#include <weku/chain/indexes_initializer.hpp>
#include <weku/chain/evaluators_initializer.hpp>
#include <weku/chain/helpers.hpp>


namespace weku { namespace chain {

using boost::container::flat_set;

class database_impl
{
   public:
      database_impl( database& self );

      database&                              _self;
      evaluator_registry< operation >        _evaluator_registry;
};

database_impl::database_impl( database& self )
   : _self(self), _evaluator_registry(self) {}

database::database()
   : _my( new database_impl(*this) ) {}

database::~database()
{
   clear_pending();
}

void database::open( const fc::path& data_dir, const fc::path& shared_mem_dir, uint64_t initial_supply, uint64_t shared_file_size, uint32_t chainbase_flags )
{
   try
   {
      // open memory database files, if not exist, will create them.
      // shared_mem.bin will contains all data, and the first object at the offset 0 of the database
      // is the object named environment_check which is used to check if the memory file is
      // created from same OS/compiler version/release or debug mode.
      // shared_mem.meta will only contains one rw_manager object which is used to contain mutex for read/write lock.
      chainbase::database::open( shared_mem_dir, chainbase_flags, shared_file_size );

      // create/read indices/tables to memory database
      indexes_initializer in_init(*this);
      in_init.initialize_indexes();
      _plugin_index_signal();
      // register evaluators
      evaluators_initializer ev_init(_my->_evaluator_registry);
      ev_init.initialize_evaluators();

      if( chainbase_flags & chainbase::database::read_write )
      {
         // if the "condition" is true, means it's at begining, need init_genesis.
         if( !find< dynamic_global_property_object >() )
            with_write_lock( [&]()
            {
               // after init_genesis, the head_block_number of gpo is set to 0;
               // since gpo's head_block_number is not set, so it's using default 0.
               // init_genesis will not trigger apply_block()
               // will also push the genesis time into processed_hardforks as hardfork 0
               init_genesis( initial_supply );
            });

         // if just after init_genesis, at this point, the block_log file will be empty.
         _block_log.open( data_dir / "block_log" );

         // Rewind all undo state. This should return us to the state at the last irreversible block.
         with_write_lock( [&]()
         {
            undo_all(); // which will clean all undo_state in all indices/tables inside database.
            FC_ASSERT( revision() == head_block_num(), "Chainbase revision does not match head block num",
               ("rev", revision())("head_block", head_block_num()) );
            validate_invariants();
         });

         // if it's first time open, means the init_genesis is just ran,
         // at this point, the head_block_num() will return 0
         if( head_block_num() )
         {
            auto head_block = _block_log.read_block_by_num( head_block_num() );
            // This assertion should be caught and a reindex should occur
            FC_ASSERT( head_block.valid() && head_block->id() == head_block_id(), "Chain state does not match block log. Please reindex blockchain." );

            // QUESTION: is it empty after database open/at this point?
            // copy the current head_block into ford_db's internal index/table as first/head object/record?
            _fork_db.start_block( *head_block );
         }
      }      
   }
   FC_CAPTURE_LOG_AND_RETHROW( (data_dir)(shared_mem_dir)(shared_file_size) )
}

// TODO: need to improve the performance of this function, since it's quite slow now.
// it's been called when user run: wekud --replay
void database::reindex( const fc::path& data_dir, const fc::path& shared_mem_dir, uint64_t shared_file_size )
{
   try
   {
      ilog( "Reindexing Blockchain" );
      // wipe here is delete two files: shared_memory.bin and shared_memory.meta
      // but keep block_log and block_log.index file.
      wipe( data_dir, shared_mem_dir, false );
      // fix reindex bug here
      //open( data_dir, shared_mem_dir, 0, shared_file_size, chainbase::database::read_write );
      open( data_dir, shared_mem_dir, STEEMIT_INIT_SUPPLY, shared_file_size, chainbase::database::read_write );
      _fork_db.reset();    // override effect of _fork_db.start_block() call in open()

      auto start = fc::time_point::now();
      STEEMIT_ASSERT( _block_log.head(), block_log_exception, "No blocks in block log. Cannot reindex an empty chain." );

      ilog( "Replaying blocks..." );

      uint64_t skip_flags =
         skip_witness_signature |
         skip_transaction_signatures |
         skip_transaction_dupe_check |
         skip_tapos_check |
         skip_merkle_check |
         skip_witness_schedule_check |
         skip_authority_check |
         skip_validate | /// no need to validate operations
         skip_validate_invariants |
         skip_block_log;

      with_write_lock( [&]()
      {
          // here 0 is file position, not block number, so itr will point to first block: block #1
         auto itr = _block_log.read_block( 0 );
         auto last_block_num = _block_log.head()->block_num();

         while( itr.first.block_num() != last_block_num )
         {
            auto cur_block_num = itr.first.block_num();
            if( cur_block_num % 100000 == 0 ) // the free memory here means free disk space of mapping file, not physical memory.
               std::cerr << "   " << double( cur_block_num * 100 ) / last_block_num << "%   " << cur_block_num << " of " << last_block_num <<
               "   (" << (get_free_memory() / (1024*1024)) << "M free)\n";
            apply_block( itr.first, skip_flags );
            itr = _block_log.read_block( itr.second ); // itr.second points to the position of next block.
         }

         apply_block( itr.first, skip_flags ); // apply last_block_num/head block number
          // set revision to head block number,
          // which in turn set all revisions to head_block_num of indices/tables contained in current database.
         set_revision( head_block_num() );
      });

      if( _block_log.head()->block_num() )
         _fork_db.start_block( *_block_log.head() );

      auto end = fc::time_point::now();
      ilog( "Done reindexing, elapsed time: ${t} sec", ("t",double((end-start).count())/1000000.0 ) );
   }
   FC_CAPTURE_AND_RETHROW( (data_dir)(shared_mem_dir) )

}

void database::wipe( const fc::path& data_dir, const fc::path& shared_mem_dir, bool include_blocks)
{
   close();
   chainbase::database::wipe( shared_mem_dir );
   if( include_blocks )
   {
      fc::remove_all( data_dir / "block_log" );
      fc::remove_all( data_dir / "block_log.index" );
   }
}


void database::close(bool rewind)
{
   try
   {
      // Since pop_block() will move tx's in the popped blocks into pending,
      // we have to clear_pending() after we're done popping to get a clean
      // DB state (issue #336).
      clear_pending();

      chainbase::database::flush();
      chainbase::database::close();

      _block_log.close();

      _fork_db.reset();
   }
   FC_CAPTURE_AND_RETHROW()
}

uint32_t database::last_hardfork() const
{
   return get_hardfork_property_object().last_hardfork;
}

void database::last_hardfork(uint32_t hardfork) 
{
   modify( get_hardfork_property_object(), [&]( hardfork_property_object& hfp )
   {
      hfp.last_hardfork = hardfork;
   });
}

hardfork_votes_type database::next_hardfork_votes() const{
   return _next_hardfork_votes;
}

void database::next_hardfork_votes(hardfork_votes_type next_hardfork_votes)
{
    _next_hardfork_votes = next_hardfork_votes;
}

// if the block is in fork_db or in block_log, return true, otherwise return false.
bool database::is_known_block( const block_id_type& id )const
{ try {
   return fetch_block_by_id( id ).valid();
} FC_CAPTURE_AND_RETHROW() }

/**
 * Only return true *if* the transaction has not expired or been invalidated. If this
 * method is called with a VERY old transaction we will return false, they should
 * query things by blocks if they are that old.
 */
bool database::is_known_transaction( const transaction_id_type& id )const
{ try {
   const auto& trx_idx = get_index<transaction_index>().indices().get<by_trx_id>();
   return trx_idx.find( id ) != trx_idx.end();
} FC_CAPTURE_AND_RETHROW() }

block_id_type database::find_block_id_for_num( uint32_t block_num )const
{
   try
   {
      if( block_num == 0 )
         return block_id_type();

      // Reversible blocks are *usually* in the TAPOS buffer.  Since this
      // is the fastest check, we do it first.
      block_summary_id_type bsid = block_num & 0xFFFF;
      const block_summary_object* bs = find< block_summary_object, by_id >( bsid );
      if( bs != nullptr )
      {
         if( protocol::block_header::num_from_id(bs->block_id) == block_num )
            return bs->block_id;
      }

      // Next we query the block log.   Irreversible blocks are here.
      auto b = _block_log.read_block_by_num( block_num );
      if( b.valid() )
         return b->id();

      // Finally we query the fork DB.
      shared_ptr< fork_item > fitem = _fork_db.fetch_block_on_main_branch_by_number( block_num );
      if( fitem )
         return fitem->id;

      return block_id_type();
   }
   FC_CAPTURE_AND_RETHROW( (block_num) )
}

block_id_type database::get_block_id_for_num( uint32_t block_num )const
{
   block_id_type bid = find_block_id_for_num( block_num );
   FC_ASSERT( bid != block_id_type() );
   return bid;
}

optional<signed_block> database::fetch_block_by_id( const block_id_type& id )const
{ try {
   auto b = _fork_db.fetch_block( id );
   if( !b )
   {
      auto tmp = _block_log.read_block_by_num( protocol::block_header::num_from_id( id ) );

      if( tmp && tmp->id() == id )
         return tmp;

      tmp.reset();
      return tmp;
   }

   return b->data;
} FC_CAPTURE_AND_RETHROW() }

optional<signed_block> database::fetch_block_by_number( uint32_t block_num )const
{ try {
   optional< signed_block > b;

   auto results = _fork_db.fetch_block_by_number( block_num );
   if( results.size() == 1 )
      b = results[0]->data;
   else
      b = _block_log.read_block_by_num( block_num );

   return b;
} FC_LOG_AND_RETHROW() }

const signed_transaction database::get_recent_transaction( const transaction_id_type& trx_id ) const
{ try {
   auto& index = get_index<transaction_index>().indices().get<by_trx_id>();
   auto itr = index.find(trx_id);
   FC_ASSERT(itr != index.end());
   signed_transaction trx;
   fc::raw::unpack( itr->packed_trx, trx ); // transaction is packed into bytes in transaction_index
   return trx;;
} FC_CAPTURE_AND_RETHROW() }

std::vector< block_id_type > database::get_block_ids_on_fork( block_id_type head_of_fork ) const
{ try {
   pair<fork_database::branch_type, fork_database::branch_type> branches = _fork_db.fetch_branch_from(head_block_id(), head_of_fork);

   // if below condition is not satisfield, means the system cannot find common ancestor between head_block_id() and head_of_fork
   // should return sth like:
    // branch1: head_block_id(), ..., a3, a2, a1.
    // branch2: head_of_fork, ..., b3, b2, b1.
    // and a1.previous_id() == b1.previous_id() if found common ancestor

   if( !((branches.first.back()->previous_id() == branches.second.back()->previous_id())) )
   {// should never happen in production mode
      edump( (head_of_fork)
             (head_block_id())
             (branches.first.size())
             (branches.second.size()) );
      // print error, and throw exception (terminate system, only applies in debug mode)
      assert(branches.first.back()->previous_id() == branches.second.back()->previous_id());
   }
   std::vector< block_id_type > result;
   for( const item_ptr& fork_block : branches.second )
      result.emplace_back(fork_block->id);
   result.emplace_back(branches.first.back()->previous_id()); // add common ancestor id.
   return result;
} FC_CAPTURE_AND_RETHROW() }

chain_id_type database::get_chain_id() const
{
   return STEEMIT_CHAIN_ID;
}

const witness_object& database::get_witness( const account_name_type& name ) const
{ try {
   return get< witness_object, by_name >( name );
} FC_CAPTURE_AND_RETHROW( (name) ) }

const witness_object* database::find_witness( const account_name_type& name ) const
{
   return find< witness_object, by_name >( name );
}

const account_object& database::get_account( const account_name_type& name )const
{ try {
   return get< account_object, by_name >( name );
} FC_CAPTURE_AND_RETHROW( (name) ) }

const account_object* database::find_account( const account_name_type& name )const
{
   return find< account_object, by_name >( name );
}

const comment_object& database::get_comment( const account_name_type& author, const shared_string& permlink )const
{ try {
   return get< comment_object, by_permlink >( boost::make_tuple( author, permlink ) );
} FC_CAPTURE_AND_RETHROW( (author)(permlink) ) }

const comment_object* database::find_comment( const account_name_type& author, const shared_string& permlink )const
{
   return find< comment_object, by_permlink >( boost::make_tuple( author, permlink ) );
}

const comment_object& database::get_comment( const account_name_type& author, const string& permlink )const
{ try {
   return get< comment_object, by_permlink >( boost::make_tuple( author, permlink) );
} FC_CAPTURE_AND_RETHROW( (author)(permlink) ) }

const comment_object* database::find_comment( const account_name_type& author, const string& permlink )const
{
   return find< comment_object, by_permlink >( boost::make_tuple( author, permlink ) );
}

const escrow_object& database::get_escrow( const account_name_type& name, uint32_t escrow_id )const
{ try {
   return get< escrow_object, by_from_id >( boost::make_tuple( name, escrow_id ) );
} FC_CAPTURE_AND_RETHROW( (name)(escrow_id) ) }


const node_property_object& database::get_node_properties() const
{
   return _node_property_object;
}

const feed_history_object& database::get_feed_history()const
{ try {
   return get< feed_history_object >();
} FC_CAPTURE_AND_RETHROW() }

const dynamic_global_property_object&database::get_dynamic_global_properties() const
{ try {    
   return get< dynamic_global_property_object >();
} FC_CAPTURE_AND_RETHROW() }

const witness_schedule_object& database::get_witness_schedule_object()const
{ try {
   return get< witness_schedule_object >();
} FC_CAPTURE_AND_RETHROW() }

const hardfork_property_object& database::get_hardfork_property_object()const
{ try {
   return get< hardfork_property_object >();
} FC_CAPTURE_AND_RETHROW() }

// not doing actual calc, just get data, should be renamed to get_discussion_payout_time
const time_point_sec database::calculate_discussion_payout_time( const comment_object& comment )const
{
   if( has_hardfork( STEEMIT_HARDFORK_0_17 ) || comment.parent_author == STEEMIT_ROOT_POST_PARENT )
      return comment.cashout_time;
   else
      return get< comment_object >( comment.root_comment ).cashout_time;
}

const reward_fund_object& database::get_reward_fund( const comment_object& c ) const
{
   return get< reward_fund_object, by_name >( STEEMIT_POST_REWARD_FUND_NAME );
}

void database::add_checkpoints( const flat_map< uint32_t, block_id_type >& checkpts )
{
   for( const auto& i : checkpts )
      _checkpoints[i.first] = i.second;
}

// bool database::before_last_checkpoint()const
// {
//    return (_checkpoints.size() > 0) && (_checkpoints.rbegin()->first >= head_block_num());
// }

/**
 * Push block "may fail" in which case every partial change is unwound.  After
 * push block is successful the block is appended to the chain database on disk.
 *
 * @return true if we switched forks as a result of this push.
 */
bool database::push_block(const signed_block& new_block, uint32_t skip)
{
   //fc::time_point begin_time = fc::time_point::now();

   bool result;
   detail::with_skip_flags( *this, skip, [&]()
   {
      with_write_lock( [&]()
      {
         detail::without_pending_transactions( *this, std::move(_pending_tx), [&]()
         {
            try
            {
               result = _push_block(new_block);
            }
            FC_CAPTURE_AND_RETHROW( (new_block) )
         });
      });
   });

   //fc::time_point end_time = fc::time_point::now();
   //fc::microseconds dt = end_time - begin_time;
   //if( ( new_block.block_num() % 10000 ) == 0 )
   //   ilog( "push_block ${b} took ${t} microseconds", ("b", new_block.block_num())("t", dt.count()) );
   return result;
}

// TODO: very important code related to consensus, need to refactory to improve readability
bool database::_push_block(const signed_block& new_block)
{ try {
   uint32_t skip = get_node_properties().skip_flags;
   //uint32_t skip_undo_db = skip & skip_undo_block;

   if( !(skip&skip_fork_db) )
   {
      // _fork_db.push_block will return the head block of current longest chain in fork_db.
      shared_ptr<fork_item> new_head = _fork_db.push_block(new_block);
      maybe_warn_multiple_production(*this, new_head->num );

      //If the head block from the longest chain does not build off of the current head, we need to switch forks.
      if( new_head->data.previous != head_block_id() )
      {
         //If the newly pushed block is the same height as head, we get head back in new_head
         //Only switch forks if new_head is actually higher than head
         if( new_head->data.block_num() > head_block_num() )
         {
            wlog( "Switching to fork: ${id}", ("id",new_head->data.id()) );

            // get two branches, which shared same parent, not include parent, stored reversely.
            // such as: head_block_id(), ..., a3, a2, a1 (a3.block_num > a2.block_num > a1.block_num)
            auto branches = _fork_db.fetch_branch_from(new_head->data.id(), head_block_id());

            // pop blocks until we hit the forked block ??
            // pop blocks until we hit the commen ancestor block of these two branches ??
            // abandon blocks on shorter branch, and add blocks from longer branch later.
            while( head_block_id() != branches.second.back()->data.previous )
               pop_block(); // pop block in fork_db

               // add blocks from longer branch based on common ancestor (since blocks on shorter branch are already abandoned above.)
            // push all blocks on the new fork
            for( auto ritr = branches.first.rbegin(); ritr != branches.first.rend(); ++ritr )
            {
                // ilog( "pushing blocks from fork ${n} ${id}", ("n",(*ritr)->data.block_num())("id",(*ritr)->data.id()) );
                optional<fc::exception> except;
                try
                {
                   auto session = start_undo_session( true );
                   apply_block( (*ritr)->data, skip );
                   session.push();
                }
                catch ( const fc::exception& e ) { except = e; }

                // if any error during appling all blocks on the new branch, the system will roll back to previous branch.
                if( except )
                {
                   wlog( "exception thrown while switching forks ${e}", ("e",except->to_detail_string() ) );

                   // remove the rest of branches.first from the fork_db, those blocks are invalid
                   // for example: fork_branch is: new_head, ..., b5, b4, b3, b2, b1.
                   // if exception happens while applying block b3, then b3, b4, b5, ... new_head will all be removed.
                   while( ritr != branches.first.rend() )
                   {
                      _fork_db.remove( (*ritr)->data.id() );
                      ++ritr;
                   }
                   // reset head back to head_block_id()
                   _fork_db.set_head( branches.second.front() );

                   // pop all blocks from the bad fork
                   while( head_block_id() != branches.second.back()->data.previous )
                      pop_block();

                   // restore all blocks from the good fork
                   for( auto ritr = branches.second.rbegin(); ritr != branches.second.rend(); ++ritr )
                   {
                      auto session = start_undo_session( true );
                      apply_block( (*ritr)->data, skip );
                      session.push();
                   }
                   throw *except;
                }
            }
            return true;
         }
         else
            return false;
      }
   }

   try
   {
      auto session = start_undo_session( true );
      apply_block(new_block, skip);
      // when session complete, leave all undo_states on the stack instead of undo them.
      session.push();
   }
   catch( const fc::exception& e )
   {
      elog("Failed to push new block:\n${e}", ("e", e.to_detail_string()));
      _fork_db.remove(new_block.id());
      throw;
   }

   return false;
} FC_CAPTURE_AND_RETHROW() }

/**
 * Attempts to push the transaction into the pending queue
 *
 * When called to push a locally generated transaction, set the skip_block_size_check bit on the skip argument. This
 * will allow the transaction to be pushed even if it causes the pending block size to exceed the maximum block size.
 * Although the transaction will probably not propagate further now, as the peers are likely to have their pending
 * queues full as well, it will be kept in the queue to be propagated later when a new block flushes out the pending
 * queues.
 */
void database::push_transaction( const signed_transaction& trx, uint32_t skip )
{
   try
   {
      try
      {
         FC_ASSERT( fc::raw::pack_size(trx) <= (get_dynamic_global_properties().maximum_block_size - 256) );
         set_producing( true );
         detail::with_skip_flags( *this, skip,
            [&]()
            {
               with_write_lock( [&]()
               {
                  _push_transaction( trx );
               });
            });
         set_producing( false );
      }
      catch( ... )
      {
         set_producing( false );
         throw;
      }
   }
   FC_CAPTURE_AND_RETHROW( (trx) )
}

void database::_push_transaction( const signed_transaction& trx )
{
   // If this is the first transaction pushed after applying a block, start a new undo session.
   // This allows us to quickly rewind to the clean state of the head block, in case a new block arrives.
   if( !_pending_tx_session.valid() )
      _pending_tx_session = start_undo_session( true );

   // Create a temporary undo session as a child of _pending_tx_session.
   // The temporary session will be discarded by the destructor if
   // _apply_transaction fails.  If we make it to merge(), we
   // apply the changes.

   auto temp_session = start_undo_session( true );
   _apply_transaction( trx );
   _pending_tx.push_back( trx );

   //notify_changed_objects();
   // The transaction applied successfully. Merge its changes into the pending block session.
   temp_session.squash();

   // notify anyone listening to pending transactions
   notify_on_pending_transaction( trx );
}

signed_block database::generate_block(
   fc::time_point_sec when,
   const account_name_type& witness_owner,
   const fc::ecc::private_key& block_signing_private_key,
   uint32_t skip /* = 0 */
   )
{
   signed_block result;
   detail::with_skip_flags( *this, skip, [&]()
   {
      try
      {
         result = _generate_block( when, witness_owner, block_signing_private_key );
      }
      FC_CAPTURE_AND_RETHROW( (witness_owner) )
   });
   return result;
}


signed_block database::_generate_block(
   fc::time_point_sec when,
   const account_name_type& witness_owner,
   const fc::ecc::private_key& block_signing_private_key
   )
{
   uint32_t skip = get_node_properties().skip_flags;
   uint32_t slot_num = get_slot_at_time( when );
   FC_ASSERT( slot_num > 0 );
   string scheduled_witness = get_scheduled_witness( slot_num );
   FC_ASSERT( scheduled_witness == witness_owner );

   const auto& witness_obj = get_witness( witness_owner );

   if( !(skip & skip_witness_signature) )
      FC_ASSERT( witness_obj.signing_key == block_signing_private_key.get_public_key() );

   static const size_t max_block_header_size = fc::raw::pack_size( signed_block_header() ) + 4;
   auto maximum_block_size = get_dynamic_global_properties().maximum_block_size; //STEEMIT_MAX_BLOCK_SIZE;
   size_t total_block_size = max_block_header_size;

   signed_block pending_block;

   with_write_lock( [&]()
   {
      //
      // The following code throws away existing pending_tx_session and
      // rebuilds it by re-applying pending transactions.
      //
      // This rebuild is necessary because pending transactions' validity
      // and semantics may have changed since they were received, because
      // time-based semantics are evaluated based on the current block
      // time.  These changes can only be reflected in the database when
      // the value of the "when" variable is known, which means we need to
      // re-apply pending transactions in this method.
      //
      _pending_tx_session.reset();
      _pending_tx_session = start_undo_session( true );

      uint64_t postponed_tx_count = 0;
      // pop pending state (reset to head block state)
      for( const signed_transaction& tx : _pending_tx )
      {
         // Only include transactions that have not expired yet for currently generating block,
         // this should clear problem transactions and allow block production to continue

         if( tx.expiration < when )
            continue;

         uint64_t new_total_size = total_block_size + fc::raw::pack_size( tx );

         // postpone transaction if it would make block too big
         if( new_total_size >= maximum_block_size )
         {
            postponed_tx_count++;
            continue;
         }

         try
         {
            auto temp_session = start_undo_session( true );
            _apply_transaction( tx );
            temp_session.squash();

            total_block_size += fc::raw::pack_size( tx );
            pending_block.transactions.push_back( tx );
         }
         catch ( const fc::exception& e )
         {
            // Do nothing, transaction will not be re-applied
            wlog( "Transaction was not processed while generating block due to ${e}", ("e", e) );
            wlog( "The transaction was ${t}", ("t", tx) );
         }
      }
      if( postponed_tx_count > 0 )
      {
         wlog( "Postponed ${n} transactions due to block size limit", ("n", postponed_tx_count) );
      }

      _pending_tx_session.reset();
   });

   // We have temporarily broken the invariant that
   // _pending_tx_session is the result of applying _pending_tx, as
   // _pending_tx now consists of the set of postponed transactions.
   // However, the push_block() call below will re-create the
   // _pending_tx_session.

   pending_block.previous = head_block_id();
   pending_block.timestamp = when;
   pending_block.transaction_merkle_root = pending_block.calculate_merkle_root();
   pending_block.witness = witness_owner;
   if( has_hardfork( STEEMIT_HARDFORK_0_19 ) )
   {
      const auto& witness = get_witness( witness_owner );

      if( witness.running_version != STEEMIT_BLOCKCHAIN_VERSION )
         pending_block.extensions.insert( block_header_extensions( STEEMIT_BLOCKCHAIN_VERSION ) );

      const auto& hfp = get_hardfork_property_object();

      if( hfp.current_hardfork_version < STEEMIT_BLOCKCHAIN_HARDFORK_VERSION // Binary is newer hardfork than has been applied
         && ( witness.hardfork_version_vote != _hardfork_versions[ hfp.last_hardfork + 1 ] || witness.hardfork_time_vote != _hardfork_times[ hfp.last_hardfork + 1 ] ) ) // Witness vote does not match binary configuration
      {
         // Make vote match binary configuration
         pending_block.extensions.insert( block_header_extensions( hardfork_version_vote( _hardfork_versions[ hfp.last_hardfork + 1 ], _hardfork_times[ hfp.last_hardfork + 1 ] ) ) );
      }
      else if( hfp.current_hardfork_version == STEEMIT_BLOCKCHAIN_HARDFORK_VERSION // Binary does not know of a new hardfork
         && witness.hardfork_version_vote > STEEMIT_BLOCKCHAIN_HARDFORK_VERSION ) // Voting for hardfork in the future, that we do not know of...
      {
         // Make vote match binary configuration. This is vote to not apply the new hardfork.
         pending_block.extensions.insert( block_header_extensions( hardfork_version_vote( _hardfork_versions[ hfp.last_hardfork ], _hardfork_times[ hfp.last_hardfork ] ) ) );
      }
   }

   if( !(skip & skip_witness_signature) )
      pending_block.sign( block_signing_private_key );

   // TODO:  Move this to _push_block() so session is restored.
   if( !(skip & skip_block_size_check) )
   {
      FC_ASSERT( fc::raw::pack_size(pending_block) <= STEEMIT_MAX_BLOCK_SIZE );
   }

   push_block( pending_block, skip );

   return pending_block;
}

/**
 * Removes the most recent block from the database and
 * undoes any changes it made.
 */
void database::pop_block()
{
   try
   {
      _pending_tx_session.reset();
      auto head_id = head_block_id();

      /// save the head block so we can recover its transactions
      optional<signed_block> head_block = fetch_block_by_id( head_id );
      STEEMIT_ASSERT( head_block.valid(), pop_empty_chain, "there are no blocks to pop" );

      _fork_db.pop_block();
      undo();

      _popped_tx.insert( _popped_tx.begin(), head_block->transactions.begin(), head_block->transactions.end() );

   }
   FC_CAPTURE_AND_RETHROW()
}

void database::clear_pending()
{
   try
   {
      assert( (_pending_tx.size() == 0) || _pending_tx_session.valid() );
      _pending_tx.clear();
      _pending_tx_session.reset();
   }
   FC_CAPTURE_AND_RETHROW()
}

void database::notify_pre_apply_operation( operation_notification& note )
{
   note.trx_id       = _current_trx_id;
   note.block        = _current_block_num;
   note.trx_in_block = _current_trx_in_block;
   note.op_in_trx    = _current_op_in_trx;

   STEEMIT_TRY_NOTIFY( pre_apply_operation, note )
}

void database::notify_post_apply_operation( const operation_notification& note )
{
   STEEMIT_TRY_NOTIFY( post_apply_operation, note )
}

const void database::push_virtual_operation( const operation& op, bool force )
{
   if( !force )
   {
      #if defined( IS_LOW_MEM ) && ! defined( IS_TEST_NET )
      return;
      #endif
   }

   FC_ASSERT( is_virtual_operation( op ) );
   operation_notification note(op);
   notify_pre_apply_operation( note );
   notify_post_apply_operation( note );
}

void database::notify_applied_block( const signed_block& block )
{
   STEEMIT_TRY_NOTIFY( applied_block, block )
}

void database::notify_on_pending_transaction( const signed_transaction& tx )
{
   STEEMIT_TRY_NOTIFY( on_pending_transaction, tx )
}

void database::notify_on_pre_apply_transaction( const signed_transaction& tx )
{
   STEEMIT_TRY_NOTIFY( on_pre_apply_transaction, tx )
}

void database::notify_on_applied_transaction( const signed_transaction& tx )
{
   STEEMIT_TRY_NOTIFY( on_applied_transaction, tx )
}

account_name_type database::get_scheduled_witness( uint32_t slot_num )const
{
   const dynamic_global_property_object& dpo = get_dynamic_global_properties();
   const witness_schedule_object& wso = get_witness_schedule_object();
   uint64_t current_aslot = dpo.current_aslot + slot_num;
   return wso.current_shuffled_witnesses[ current_aslot % wso.num_scheduled_witnesses ];
}

fc::time_point_sec database::get_slot_time(uint32_t slot_num)const
{
   slot s(*this);
   return s.get_slot_time(slot_num);
}

uint32_t database::get_slot_at_time(fc::time_point_sec when)const
{
   slot s(*this);
   return s.get_slot_at_time(when);
}



/**
 * @param to_account - the account to receive the new vesting shares
 * @param STEEM - STEEM to be converted to vesting shares
 */
asset database::create_vesting( const account_object& to_account, asset steem, bool to_reward_balance )
{
   fund_processor fp(*this);
   return fp.create_vesting(to_account, steem, to_reward_balance);
}

// TODO: need to figure out the exact logic of adjust proxied witness votes function groups.
void database::adjust_proxied_witness_votes( const account_object& a,
                                   const std::array< share_type, STEEMIT_MAX_PROXY_RECURSION_DEPTH+1 >& delta,
                                   int depth )
{
   if( a.proxy != STEEMIT_PROXY_TO_SELF_ACCOUNT )
   {
      /// nested proxies are not supported, vote will not propagate
      if( depth >= STEEMIT_MAX_PROXY_RECURSION_DEPTH )
         return;

      const auto& proxy = get_account( a.proxy );

      modify( proxy, [&]( account_object& a )
      {
         for( int i = STEEMIT_MAX_PROXY_RECURSION_DEPTH - depth - 1; i >= 0; --i )
         {
            a.proxied_vsf_votes[i+depth] += delta[i];
         }
      } );

      adjust_proxied_witness_votes( proxy, delta, depth + 1 );
   }
   else
   {
      share_type total_delta = 0;
      for( int i = STEEMIT_MAX_PROXY_RECURSION_DEPTH - depth; i >= 0; --i )
         total_delta += delta[i];
      adjust_witness_votes( a, total_delta );
   }
}

void database::adjust_proxied_witness_votes( const account_object& a, share_type delta, int depth )
{
   if( a.proxy != STEEMIT_PROXY_TO_SELF_ACCOUNT )
   {
      /// nested proxies are not supported, vote will not propagate
      if( depth >= STEEMIT_MAX_PROXY_RECURSION_DEPTH )
         return;

      const auto& proxy = get_account( a.proxy );

      modify( proxy, [&]( account_object& a )
      {
         a.proxied_vsf_votes[depth] += delta;
      } );

      adjust_proxied_witness_votes( proxy, delta, depth + 1 );
   }
   else
   {
     adjust_witness_votes( a, delta );
   }
}

void database::adjust_witness_votes( const account_object& a, share_type delta )
{
   const auto& vidx = get_index< witness_vote_index >().indices().get< by_account_witness >();
   auto itr = vidx.lower_bound( boost::make_tuple( a.id, witness_id_type() ) );
   while( itr != vidx.end() && itr->account == a.id )
   {
      adjust_witness_vote( get(itr->witness), delta );
      ++itr;
   }
}

void database::adjust_witness_vote( const witness_object& witness, share_type delta )
{
   const witness_schedule_object& wso = get_witness_schedule_object();
   modify( witness, [&]( witness_object& w )
   {
      auto delta_pos = w.votes.value * (wso.current_virtual_time - w.virtual_last_update);
      w.virtual_position += delta_pos;

      w.virtual_last_update = wso.current_virtual_time;
      w.votes += delta;
      // TODO: need update votes type to match total_vesting_shares type
      FC_ASSERT( w.votes <= get_dynamic_global_properties().total_vesting_shares.amount, "", ("w.votes", w.votes)("props",get_dynamic_global_properties().total_vesting_shares) );

      if( has_hardfork( STEEMIT_HARDFORK_0_2 ) )
         w.virtual_scheduled_time = w.virtual_last_update + (VIRTUAL_SCHEDULE_LAP_LENGTH2 - w.virtual_position)/(w.votes.value+1);
      else
         w.virtual_scheduled_time = w.virtual_last_update + (VIRTUAL_SCHEDULE_LAP_LENGTH - w.virtual_position)/(w.votes.value+1);

      /** witnesses with a low number of votes could overflow the time field and end up with a scheduled time in the past */
      if( has_hardfork( STEEMIT_HARDFORK_0_4 ) )
      {
         if( w.virtual_scheduled_time < wso.current_virtual_time )
            w.virtual_scheduled_time = fc::uint128::max_value();
      }
   } );
}




/**
 * This method updates total_reward_shares2 on DGPO, and children_rshares2 on comments, when a comment's rshares2 changes
 * from old_rshares2 to new_rshares2.  Maintaining invariants that children_rshares2 is the sum of all descendants' rshares2,
 * and dgpo.total_reward_shares2 is the total number of rshares2 outstanding.
 */
void database::adjust_rshares2( const comment_object& c, fc::uint128_t old_rshares2, fc::uint128_t new_rshares2 )
{

   const auto& dgpo = get_dynamic_global_properties();
   modify( dgpo, [&]( dynamic_global_property_object& p )
   {
      p.total_reward_shares2 -= old_rshares2;
      p.total_reward_shares2 += new_rshares2;
   } );
}

void process_savings_withdraws(itemp_database& db)
{
  const auto& idx = db.get_index< savings_withdraw_index >().indices().get< by_complete_from_rid >();
  auto itr = idx.begin();
  while( itr != idx.end() ) {
     if( itr->complete > db.head_block_time() )
        break;
     db.adjust_balance( db.get_account( itr->to ), itr->amount );

     db.modify( db.get_account( itr->from ), [&]( account_object& a )
     {
        a.savings_withdraw_requests--;
     });

     db.push_virtual_operation( fill_transfer_from_savings_operation( itr->from, itr->to, itr->amount, itr->request_id, to_string( itr->memo) ) );

     db.remove( *itr );
     itr = idx.begin();
  }
}

asset database::to_sbd( const asset& steem )const
{
   return util::to_sbd( get_feed_history().current_median_history, steem );
}

asset database::to_steem( const asset& sbd )const
{
   return util::to_steem( get_feed_history().current_median_history, sbd );
}

time_point_sec database::head_block_time()const
{
   return get_dynamic_global_properties().time;
}

uint32_t database::head_block_num()const
{
   return get_dynamic_global_properties().head_block_number;
}

block_id_type database::head_block_id()const
{
   return get_dynamic_global_properties().head_block_id;
}

node_property_object& database::node_properties()
{
   return _node_property_object;
}

uint32_t database::last_non_undoable_block_num() const
{
   return get_dynamic_global_properties().last_irreversible_block_num;
}

void database::set_custom_operation_interpreter( const std::string& id, std::shared_ptr< custom_operation_interpreter > registry )
{
   bool inserted = _custom_operation_interpreters.emplace( id, registry ).second;
   // This assert triggering means we're mis-configured (multiple registrations of custom JSON evaluator for same ID)
   FC_ASSERT( inserted );
}

std::shared_ptr< custom_operation_interpreter > database::get_custom_json_evaluator( const std::string& id )
{
   auto it = _custom_operation_interpreters.find( id );
   if( it != _custom_operation_interpreters.end() )
      return it->second;
   return std::shared_ptr< custom_operation_interpreter >();
}

const std::string& database::get_json_schema()const
{
   return _json_schema;
}

void database::init_genesis( uint64_t init_supply )
{
   genesis_processor gp(*this);
   gp.init_genesis(init_supply);
}


// void database::validate_transaction( const signed_transaction& trx )
// {
//    database::with_write_lock( [&]()
//    {
//       auto session = start_undo_session( true );
//       _apply_transaction( trx );
//       session.undo();
//    });
// }

void database::set_flush_interval( uint32_t flush_blocks )
{
   _flush_blocks = flush_blocks;
   _next_flush_block = 0;
}

//////////////////// private methods ////////////////////

void database::apply_block( const signed_block& next_block, uint32_t skip )
{ try {
   //fc::time_point begin_time = fc::time_point::now();

   auto block_num = next_block.block_num();
   if( _checkpoints.size() && _checkpoints.rbegin()->second != block_id_type() )
   {
      auto itr = _checkpoints.find( block_num );
      if( itr != _checkpoints.end() )
         FC_ASSERT( next_block.id() == itr->second, "Block did not match checkpoint", ("checkpoint",*itr)("block_id",next_block.id()) );

      if( _checkpoints.rbegin()->first >= block_num )
         skip = skip_witness_signature
              | skip_transaction_signatures
              | skip_transaction_dupe_check
              | skip_fork_db
              | skip_block_size_check
              | skip_tapos_check
              | skip_authority_check
              /* | skip_merkle_check While blockchain is being downloaded, txs need to be validated against block headers */
              | skip_undo_history_check
              | skip_witness_schedule_check
              | skip_validate
              | skip_validate_invariants
              ;
   }

   detail::with_skip_flags( *this, skip, [&]()
   {
      _apply_block( next_block );
   } );

   /*try
   {
   /// check invariants
   if( is_producing() || !( skip & skip_validate_invariants ) )
      validate_invariants();
   }
   FC_CAPTURE_AND_RETHROW( (next_block) );*/

   //fc::time_point end_time = fc::time_point::now();
   //fc::microseconds dt = end_time - begin_time;

   // QUESTION: What is below section of code for?
   if( _flush_blocks != 0 )
   {
      if( _next_flush_block == 0 )
      {
         uint32_t lep = block_num + 1 + _flush_blocks * 9 / 10;
         uint32_t rep = block_num + 1 + _flush_blocks;

         // use time_point::now() as RNG source to pick block randomly between lep and rep
         uint32_t span = rep - lep;
         uint32_t x = lep;
         if( span > 0 )
         {
            uint64_t now = uint64_t( fc::time_point::now().time_since_epoch().count() );
            x += now % span;
         }
         _next_flush_block = x;
         ilog( "Next flush scheduled at block ${b}", ("b", x) );
      }

      if( _next_flush_block == block_num )
      {
         _next_flush_block = 0;
         ilog( "Flushing database shared memory at block ${b}", ("b", block_num) );
         chainbase::database::flush();
      }
   }

   show_free_memory( false );

} FC_CAPTURE_AND_RETHROW( (next_block) ) }

void database::show_free_memory( bool force )
{
   uint32_t free_gb = uint32_t( get_free_memory() / (1024*1024*1024) );
   if( force || (free_gb < _last_free_gb_printed) || (free_gb > _last_free_gb_printed+1) )
   {
      ilog( "Free memory is now ${n}G", ("n", free_gb) );
      _last_free_gb_printed = free_gb;
   }

   if( free_gb == 0 )
   {
      uint32_t free_mb = uint32_t( get_free_memory() / (1024*1024) );

      if( free_mb <= 100 && head_block_num() % 10 == 0 )
         elog( "Free memory is now ${n}M. Increase shared file size immediately!" , ("n", free_mb) );
   }
}

// Most important function in the system.
void database::_apply_block( const signed_block& next_block )
{ try {
   uint32_t next_block_num = next_block.block_num();
   //block_id_type next_block_id = next_block.id();

   uint32_t skip = get_node_properties().skip_flags;

   if( !( skip & skip_merkle_check ) )
   {
      auto merkle_root = next_block.calculate_merkle_root();

      try
      {
         FC_ASSERT( next_block.transaction_merkle_root == merkle_root, "Merkle check failed", ("next_block.transaction_merkle_root",next_block.transaction_merkle_root)("calc",merkle_root)("next_block",next_block)("id",next_block.id()) );
      }
      catch( fc::assert_exception& e )
      {
         const auto& merkle_map = get_shared_db_merkle();
         auto itr = merkle_map.find( next_block_num );

         if( itr == merkle_map.end() || itr->second != merkle_root )
            throw e;
      }
   }

   block_header_validator bhv(*this);
   const witness_object& signing_witness = bhv.validate_block_header(skip, next_block);

   _current_block_num    = next_block_num;
   _current_trx_in_block = 0;

   const auto& gprops = get_dynamic_global_properties();
   auto block_size = fc::raw::pack_size( next_block );
   if( has_hardfork( STEEMIT_HARDFORK_0_12 ) )
   {
      FC_ASSERT( block_size <= gprops.maximum_block_size, "Block Size is too Big", ("next_block_num",next_block_num)("block_size", block_size)("max",gprops.maximum_block_size) );
   }

   if( block_size < STEEMIT_MIN_BLOCK_SIZE )
   {
      elog( "Block size is too small",
         ("next_block_num",next_block_num)("block_size", block_size)("min",STEEMIT_MIN_BLOCK_SIZE)
      );
   }

   /// modify current witness so transaction evaluators can know who included the transaction,
   /// this is mostly for POW operations which must pay the current_witness
   modify( gprops, [&]( dynamic_global_property_object& dgp ){
      dgp.current_witness = next_block.witness;
   });

   /// parse witness version reporting
   process_header_extensions( next_block );

   for( const auto& trx : next_block.transactions )
   {
      /* We do not need to push the undo state for each transaction
       * because they either all apply and are valid or the
       * entire block fails to apply.  We only need an "undo" state
       * for transactions when validating broadcast transactions or
       * when building a block.
       */
      apply_transaction( trx, skip );
      ++_current_trx_in_block;
   }

   block_applier applier(*this);
   
   // update head_block_time during this operation
   applier.update_global_dynamic_data(next_block); 
   applier.update_signing_witness(signing_witness, next_block);
   applier.update_last_irreversible_block();
   applier.create_block_summary(next_block);
   applier.clear_expired_transactions();
   applier.clear_expired_orders();
   applier.clear_expired_delegations();

   update_witness_schedule(*this);

   median_feed_updator mfu(*this);
   mfu.update_median_feed();
   
   // QUESTION: another update_virtual_supply in next 10 lines?
   applier.update_virtual_supply();

   null_account_cleaner nac(*this);
   nac.clear_null_account_balance();

   fund_processor fp(*this);
   fp.process_funds();

   conversion_processor conp(*this);
   conp.process_conversions();

   cashout_processor cop(*this);
   cop.process_comment_cashout();

   vest_withdraw_processor vwp(*this);
   vwp.process_vesting_withdrawals();

   process_savings_withdraws(*this);

   reward_processor rp(*this);
   rp.pay_liquidity_reward();

   applier.update_virtual_supply();

   account_recovery_processor arp(*this);
   arp.account_recovery_processing();

   applier.expire_escrow_ratification();

   applier.process_decline_voting_rights();

   // For WEKU, all hardforks before hardfork_20 are applied just after block 1 is saved to dish
   // this hardfork process is almost at the end of the process,
   // which also means during process of block #1, there is only harfork 0.
   hardforker hfk(*this);
   hfk.process();  

   // notify observers that the block has been applied
   notify_applied_block( next_block );

   //notify_changed_objects();
} //FC_CAPTURE_AND_RETHROW( (next_block.block_num()) )  }
FC_CAPTURE_LOG_AND_RETHROW( (next_block.block_num()) )
}

// QUESTION: no sure why use next block versions to update witness object versions.
// should the version in block header always same with witness version?
void database::process_header_extensions( const signed_block& next_block )
{
   auto itr = next_block.extensions.begin();

   while( itr != next_block.extensions.end() )
   {
      switch( itr->which() )
      {
         case 0: // void_t
            break;
         case 1: // version
         {
            auto reported_version = itr->get< version >();
            const auto& signing_witness = get_witness( next_block.witness );
            //idump( (next_block.witness)(signing_witness.running_version)(reported_version) );

            if( reported_version != signing_witness.running_version )
            {
               modify( signing_witness, [&]( witness_object& wo )
               {
                  wo.running_version = reported_version;
               });
            }
            break;
         }
         case 2: // hardfork_version vote
         {
            auto hfv = itr->get< hardfork_version_vote >();
            const auto& signing_witness = get_witness( next_block.witness );
            //idump( (next_block.witness)(signing_witness.running_version)(hfv) );

            if( hfv.hf_version != signing_witness.hardfork_version_vote || hfv.hf_time != signing_witness.hardfork_time_vote )
               modify( signing_witness, [&]( witness_object& wo )
               {
                  wo.hardfork_version_vote = hfv.hf_version;
                  wo.hardfork_time_vote = hfv.hf_time;
               });

            break;
         }
         default:
            FC_ASSERT( false, "Unknown extension in block header" );
      }

      ++itr;
   }
}

void database::apply_transaction(const signed_transaction& trx, uint32_t skip)
{
   detail::with_skip_flags( *this, skip, [&]() { _apply_transaction(trx); });
   notify_on_applied_transaction( trx );
}

void database::_apply_transaction(const signed_transaction& trx)
{ try {
   _current_trx_id = trx.id();
   uint32_t skip = get_node_properties().skip_flags;

   if( !(skip&skip_validate) )   /* issue #505 explains why this skip_flag is disabled */
      trx.validate();

   auto& trx_idx = get_index<transaction_index>();
   const chain_id_type& chain_id = STEEMIT_CHAIN_ID;
   auto trx_id = trx.id();
   // idump((trx_id)(skip&skip_transaction_dupe_check));
   FC_ASSERT( (skip & skip_transaction_dupe_check) ||
              trx_idx.indices().get<by_trx_id>().find(trx_id) == trx_idx.indices().get<by_trx_id>().end(),
              "Duplicate transaction check failed", ("trx_ix", trx_id) );

   if( !(skip & (skip_transaction_signatures | skip_authority_check) ) )
   {
      auto get_active  = [&]( const string& name ) { return authority( get< account_authority_object, by_account >( name ).active ); };
      auto get_owner   = [&]( const string& name ) { return authority( get< account_authority_object, by_account >( name ).owner );  };
      auto get_posting = [&]( const string& name ) { return authority( get< account_authority_object, by_account >( name ).posting );  };

      try
      {
         trx.verify_authority( chain_id, get_active, get_owner, get_posting, STEEMIT_MAX_SIG_CHECK_DEPTH );
      }
      catch( protocol::tx_missing_active_auth& e )
      {
         if( get_shared_db_merkle().find( head_block_num() + 1 ) == get_shared_db_merkle().end() )
            throw e;
      }
   }

   //Skip all manner of expiration and TaPoS checking if we're on block 1; It's impossible that the transaction is
   //expired, and TaPoS makes no sense as no blocks exist.
   if( BOOST_LIKELY(head_block_num() > 0) )
   {
      if( !(skip & skip_tapos_check) )
      {
         const auto& tapos_block_summary = get< block_summary_object >( trx.ref_block_num );
         //Verify TaPoS block summary has correct ID prefix, and that this block's time is not past the expiration
         STEEMIT_ASSERT( trx.ref_block_prefix == tapos_block_summary.block_id._hash[1], transaction_tapos_exception,
                    "", ("trx.ref_block_prefix", trx.ref_block_prefix)
                    ("tapos_block_summary",tapos_block_summary.block_id._hash[1]));
      }

      fc::time_point_sec now = head_block_time();

      STEEMIT_ASSERT( trx.expiration <= now + fc::seconds(STEEMIT_MAX_TIME_UNTIL_EXPIRATION), transaction_expiration_exception,
                  "", ("trx.expiration",trx.expiration)("now",now)("max_til_exp",STEEMIT_MAX_TIME_UNTIL_EXPIRATION));
      // TODO: refactory below asserts, since we has the hardfork since block 1.
      if( has_hardfork( STEEMIT_HARDFORK_0_9 ) ) // Simple solution to pending trx bug when now == trx.expiration
         STEEMIT_ASSERT( now < trx.expiration, transaction_expiration_exception, "", ("now",now)("trx.exp",trx.expiration) );
      STEEMIT_ASSERT( now <= trx.expiration, transaction_expiration_exception, "", ("now",now)("trx.exp",trx.expiration) );
   }

   //Insert transaction into unique transactions database.
   if( !(skip & skip_transaction_dupe_check) )
   {
      create<transaction_object>([&](transaction_object& transaction) {
         transaction.trx_id = trx_id;
         transaction.expiration = trx.expiration;
         fc::raw::pack( transaction.packed_trx, trx );
      });
   }

   // fire the signal (kind of event), so it will call all signal connected functions.
   notify_on_pre_apply_transaction( trx );

   //Finally process the operations
   _current_op_in_trx = 0;
   for( const auto& op : trx.operations )
   { try {
      apply_operation(op);
      ++_current_op_in_trx;
     } FC_CAPTURE_AND_RETHROW( (op) ); // QUESTION: Should we remove the transaction object created above in transaction index if error happens here?
   }
   _current_trx_id = transaction_id_type();

} FC_CAPTURE_AND_RETHROW( (trx) ) }

void database::apply_operation(const operation& op)
{
   operation_notification note(op);
   notify_pre_apply_operation( note );
   _my->_evaluator_registry.get_evaluator( op ).apply( op );
   notify_post_apply_operation( note );
}





bool database::apply_order( const limit_order_object& new_order_object )
{
   order_processor op(*this);
   return op.apply_order(new_order_object);   
}

int database::match( const limit_order_object& new_order, const limit_order_object& old_order, const price& match_price )
{
   order_processor op(*this);
   return op.match(new_order, old_order, match_price);
}

bool database::fill_order( const limit_order_object& order, const asset& pays, const asset& receives )
{
   order_processor op(*this);
   return op.fill_order(order, pays, receives);
}

void database::cancel_order( const limit_order_object& order )
{
   adjust_balance( get_account(order.seller), order.amount_for_sale() );
   remove(order);
}

void database::adjust_balance( const account_object& a, const asset& delta )
{
   balance_processor bp(*this);
   bp.adjust_balance(a, delta);
}


void database::adjust_savings_balance( const account_object& a, const asset& delta )
{
   balance_processor bp(*this);
   bp.adjust_savings_balance(a, delta);
}

void database::adjust_reward_balance( const account_object& a, const asset& delta )
{
   balance_processor bp(*this);
   bp.adjust_reward_balance(a, delta);
}

void database::adjust_supply( const asset& delta, bool adjust_vesting )
{
   balance_processor bp(*this);
   bp.adjust_supply(delta, adjust_vesting);   
}

bool database::has_hardfork( uint32_t hardfork )const
{
   return get_hardfork_property_object().last_hardfork >= hardfork;
}

// TODO: should be removed, for testing only, no production code should use this function.
void database::set_hardfork( uint32_t hardfork, bool apply_now ){}

void database::validate_invariants()const
{
   invariant_validator validator(*this);
   validator.validate();
}

} } 

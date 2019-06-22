#include <weku/chain/block_applier.hpp>
#include <weku/chain/gpo_processor.hpp>
#include <weku/chain/witness_objects.hpp>
#include <weku/chain/helpers.hpp>

namespace weku{namespace chain{

void block_applier::update_virtual_supply()
{ 
    try {
        _db.modify( _db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& dgp )
        {
            dgp.virtual_supply = dgp.current_supply
                + ( _db.get_feed_history().current_median_history.is_null() 
                ? asset( 0, STEEM_SYMBOL ) 
                : dgp.current_sbd_supply * _db.get_feed_history().current_median_history );

            auto median_price = _db.get_feed_history().current_median_history;

            if( !median_price.is_null() && _db.has_hardfork( STEEMIT_HARDFORK_0_14 ) )
            {
                auto percent_sbd = uint16_t( ( ( fc::uint128_t( ( dgp.current_sbd_supply * _db.get_feed_history().current_median_history ).amount.value ) * STEEMIT_100_PERCENT )
                    / dgp.virtual_supply.amount.value ).to_uint64() );

                if( percent_sbd <= STEEMIT_SBD_START_PERCENT )
                    dgp.sbd_print_rate = STEEMIT_100_PERCENT;
                else if( percent_sbd >= STEEMIT_SBD_STOP_PERCENT )
                    dgp.sbd_print_rate = 0;
                else
                    dgp.sbd_print_rate = ( ( STEEMIT_SBD_STOP_PERCENT - percent_sbd ) * STEEMIT_100_PERCENT ) / ( STEEMIT_SBD_STOP_PERCENT - STEEMIT_SBD_START_PERCENT );
            }
        });
    } FC_CAPTURE_AND_RETHROW() 
}


void block_applier::update_global_dynamic_data( const signed_block& b )
{ 
    gpo_processor gp(_db);
   // update head_block_time during this operation
   gp.update_global_dynamic_data(b);
}

void block_applier::create_block_summary(const signed_block& next_block)
{ try {
   block_summary_id_type sid( next_block.block_num() & 0xffff );
   _db.modify( _db.get< block_summary_object >( sid ), [&](block_summary_object& p) {
         p.block_id = next_block.id();
   });
} FC_CAPTURE_AND_RETHROW() }

void block_applier::clear_expired_orders()
{
   auto now = _db.head_block_time();
   const auto& orders_by_exp = _db.get_index<limit_order_index>().indices().get<by_expiration>();
   auto itr = orders_by_exp.begin();
   while( itr != orders_by_exp.end() && itr->expiration < now )
   {
      cancel_order(_db, *itr );
      itr = orders_by_exp.begin();
   }
}

void block_applier::clear_expired_delegations()
{
   auto now = _db.head_block_time();
   const auto& delegations_by_exp = _db.get_index< vesting_delegation_expiration_index, by_expiration >();
   auto itr = delegations_by_exp.begin();
   while( itr != delegations_by_exp.end() && itr->expiration < now )
   {
      _db.modify( _db.get_account( itr->delegator ), [&]( account_object& a )
      {
         a.delegated_vesting_shares -= itr->vesting_shares;
      });

      _db.push_virtual_operation( return_vesting_delegation_operation( itr->delegator, itr->vesting_shares ) );

      _db.remove( *itr );
      itr = delegations_by_exp.begin();
   }
}

void block_applier::clear_expired_transactions()
{
   //Look for expired transactions in the deduplication list, and remove them.
   //Transactions must have expired by at least two forking windows in order to be removed.
   auto& transaction_idx = _db.get_index< transaction_index >();
   const auto& dedupe_index = transaction_idx.indices().get< by_expiration >();
   while( ( !dedupe_index.empty() ) && ( _db.head_block_time() > dedupe_index.begin()->expiration ) )
      _db.remove( *dedupe_index.begin() );
}

void block_applier::process_decline_voting_rights()
{
   const auto& request_idx = _db.get_index< decline_voting_rights_request_index >().indices().get< by_effective_date >();
   auto itr = request_idx.begin();

   while( itr != request_idx.end() && itr->effective_date <= _db.head_block_time() )
   {
      const auto& account = _db.get(itr->account);

      /// remove all current votes
      std::array<share_type, STEEMIT_MAX_PROXY_RECURSION_DEPTH+1> delta;
      delta[0] = -account.vesting_shares.amount;
      for( int i = 0; i < STEEMIT_MAX_PROXY_RECURSION_DEPTH; ++i )
         delta[i+1] = -account.proxied_vsf_votes[i];
      _db.adjust_proxied_witness_votes( account, delta );

      clear_witness_votes(_db, account );

      _db.modify( _db.get(itr->account), [&]( account_object& a )
      {
         a.can_vote = false;
         a.proxy = STEEMIT_PROXY_TO_SELF_ACCOUNT;
      });

      _db.remove( *itr );
      itr = request_idx.begin();
   }
}

void block_applier::expire_escrow_ratification()
{
   const auto& escrow_idx = _db.get_index< escrow_index >().indices().get< by_ratification_deadline >();
   auto escrow_itr = escrow_idx.lower_bound( false );

   while( escrow_itr != escrow_idx.end() && !escrow_itr->is_approved() && escrow_itr->ratification_deadline <= _db.head_block_time() )
   {
      const auto& old_escrow = *escrow_itr;
      ++escrow_itr;

      const auto& from_account = _db.get_account( old_escrow.from );
      _db.adjust_balance( from_account, old_escrow.steem_balance );
      _db.adjust_balance( from_account, old_escrow.sbd_balance );
      _db.adjust_balance( from_account, old_escrow.pending_fee );

      _db.remove( old_escrow );
   }
}

void block_applier::update_last_irreversible_block()
{ 
    try {
        const dynamic_global_property_object& dpo = _db.get_dynamic_global_properties();

        //Prior to voting taking over, we must be more conservative...
        if( _db.head_block_num() < STEEMIT_START_MINER_VOTING_BLOCK )
        {
            _db.modify( dpo, [&]( dynamic_global_property_object& _dpo )
            {
                if ( _db.head_block_num() > STEEMIT_MAX_WITNESSES )
                    _dpo.last_irreversible_block_num = _db.head_block_num() - STEEMIT_MAX_WITNESSES;
            } );
        }
        else
        {
            const witness_schedule_object& wso = _db.get_witness_schedule_object();

            std::vector< const witness_object* > wit_objs;
            wit_objs.reserve( wso.num_scheduled_witnesses );
            //ilog("num_scheduled_witnesses: ${a}", ("a", wso.num_scheduled_witnesses));
            for( int i = 0; i < wso.num_scheduled_witnesses; i++ )
                wit_objs.push_back( &_db.get_witness( wso.current_shuffled_witnesses[i] ) );

            static_assert( STEEMIT_IRREVERSIBLE_THRESHOLD > 0, "irreversible threshold must be nonzero" );

            // QUESTION: not sure what below 3 lines is about?
            // 1 1 1 2 2 2 2 2 2 2 -> 2     .7*10 = 7
            // 1 1 1 1 1 1 1 2 2 2 -> 1
            // 3 3 3 3 3 3 3 3 3 3 -> 3

            // if wit_objs.size() < 4, then offset will be always 0.
            size_t offset = ((STEEMIT_100_PERCENT - STEEMIT_IRREVERSIBLE_THRESHOLD) * wit_objs.size() / STEEMIT_100_PERCENT);
            //ilog("offset: ${a}", ("a", offset));
            std::nth_element( wit_objs.begin(), wit_objs.begin() + offset, wit_objs.end(),
                []( const witness_object* a, const witness_object* b )
                {
                    return a->last_confirmed_block_num < b->last_confirmed_block_num;
                } );

            uint32_t new_last_irreversible_block_num = wit_objs[offset]->last_confirmed_block_num;

            if(!_db.has_hardfork(STEEMIT_HARDFORK_0_22) && offset < 1) // to fix replay stuck at: block #4735073 / #4745073 issue
                new_last_irreversible_block_num = wit_objs.back()->last_confirmed_block_num;

            //ilog("witness: ${a}", ("a", std::string(wit_objs[offset]->owner)));
            //ilog("before: new_last_irreversible_block_num: ${a}", ("a", new_last_irreversible_block_num));
            //ilog("before: dpo.last_irreversible_block_num: ${a}", ("a", dpo.last_irreversible_block_num));

            if( new_last_irreversible_block_num > dpo.last_irreversible_block_num )
            {
                _db.modify( dpo, [&]( dynamic_global_property_object& _dpo )
                {
                    _dpo.last_irreversible_block_num = new_last_irreversible_block_num;
                } );
            }
        }

        //ilog("after: dpo.last_irreversible_block_num: ${a}", ("a", dpo.last_irreversible_block_num));
        // Discards all undo history prior to revision
        _db.commit( dpo.last_irreversible_block_num );

        if( !( _db.get_node_properties().skip_flags & skip_block_log ) )
        {
            // output to block log based on new last irreverisible block num
            const auto& tmp_head = _db.get_block_log().head();
            uint64_t log_head_num = 0;

            if( tmp_head )
                log_head_num = tmp_head->block_num();
            ilog("log_head_num: ${a}", ("a", log_head_num));
            ilog("last irreversible block num: ${n}", ("n", dpo.last_irreversible_block_num));
            if( log_head_num < dpo.last_irreversible_block_num )
            {
                while( log_head_num < dpo.last_irreversible_block_num )
                {
                    std::shared_ptr< fork_item > block = _db.fork_db().fetch_block_on_main_branch_by_number( log_head_num+1 );
                    FC_ASSERT( block, "Current fork in the fork database does not contain the last_irreversible_block" );
                    _db.get_block_log().append( block->data );
                    ilog("append data to block log file.");
                    log_head_num++;
                }

                _db.get_block_log().flush();
            }
        }

        // QUESTION: not sure what exactly below function is doing
        // doing this will resize the max_size of fork_db, which in turn delete old items out of the range of new _max_size.
        _db.fork_db().set_max_size( dpo.head_block_number - dpo.last_irreversible_block_num + 1 );
    } FC_CAPTURE_AND_RETHROW() 
}


void block_applier::update_signing_witness(const witness_object& signing_witness, const signed_block& new_block)
{ 
    try {
        const dynamic_global_property_object& dpo = _db.get_dynamic_global_properties();
        uint64_t new_block_aslot = dpo.current_aslot + get_slot_at_time(_db, new_block.timestamp );

        _db.modify( signing_witness, [&]( witness_object& _wit )
        {
            _wit.last_aslot = new_block_aslot;
            _wit.last_confirmed_block_num = new_block.block_num();
            //ilog("wit.owner: ${a}",("a",std::string( _wit.owner)));
            //ilog("new block num: ${a}", ("a", new_block.block_num()));
            //ilog("last_confirmed_block_num:${a}", ("a",_wit.last_confirmed_block_num));
        } );
    } FC_CAPTURE_AND_RETHROW() 
}

}}
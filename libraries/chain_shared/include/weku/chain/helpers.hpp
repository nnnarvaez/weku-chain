#include <weku/chain/itemp_database.hpp>
#include <weku/chain/invariant_validator.hpp>
#include <weku/chain/fund_processor.hpp>
#include <weku/chain/slot.hpp>

namespace weku {namespace chain{

static void show_free_memory(itemp_database& db, uint32_t last_free_gb_printed, bool force )
{
   uint32_t free_gb = uint32_t( db.get_free_memory() / (1024*1024*1024) );
   if( force || (free_gb < last_free_gb_printed) || (free_gb > last_free_gb_printed+1) )
   {
      ilog( "Free memory is now ${n}G", ("n", free_gb) );
      last_free_gb_printed = free_gb;
   }

   if( free_gb == 0 )
   {
      uint32_t free_mb = uint32_t( db.get_free_memory() / (1024*1024) );

      if( free_mb <= 100 && db.head_block_num() % 10 == 0 )
         elog( "Free memory is now ${n}M. Increase shared file size immediately!" , ("n", free_mb) );
   }
}

static void adjust_savings_balance(itemp_database& db, const account_object& a, const asset& delta )
{
   balance_processor bp(db);
   bp.adjust_savings_balance(a, delta);
}

// this is called by `adjust_proxied_witness_votes` when account proxy to self 
void adjust_witness_votes(itemp_database& db, const account_object& a, share_type delta )
{
   const auto& vidx = db.get_index< witness_vote_index >().indices().get< by_account_witness >();
   auto itr = vidx.lower_bound( boost::make_tuple( a.id, witness_id_type() ) );
   while( itr != vidx.end() && itr->account == a.id )
   {
      adjust_witness_vote(db, db.get(itr->witness), delta );
      ++itr;
   }
}

// this updates the vote of a single witness as a result of a vote being added or removed         
static void adjust_witness_vote(itemp_database& db, const witness_object& witness, share_type delta )
{
   const witness_schedule_object& wso = db.get_witness_schedule_object();
   db.modify( witness, [&]( witness_object& w )
   {
      auto delta_pos = w.votes.value * (wso.current_virtual_time - w.virtual_last_update);
      w.virtual_position += delta_pos;

      w.virtual_last_update = wso.current_virtual_time;
      w.votes += delta;
      // TODO: need update votes type to match total_vesting_shares type
      FC_ASSERT( w.votes <= db.get_dynamic_global_properties().total_vesting_shares.amount, "", 
         ("w.votes", w.votes)("props",db.get_dynamic_global_properties().total_vesting_shares) );

      if( db.has_hardfork( STEEMIT_HARDFORK_0_19 ) )
         w.virtual_scheduled_time = w.virtual_last_update + (VIRTUAL_SCHEDULE_LAP_LENGTH2 - w.virtual_position)/(w.votes.value+1);
      else
         w.virtual_scheduled_time = w.virtual_last_update + (VIRTUAL_SCHEDULE_LAP_LENGTH - w.virtual_position)/(w.votes.value+1);

      /** witnesses with a low number of votes could overflow the time field and end up with a scheduled time in the past */
      if( db.has_hardfork( STEEMIT_HARDFORK_0_19 ) )
      {
         if( w.virtual_scheduled_time < wso.current_virtual_time )
            w.virtual_scheduled_time = fc::uint128::max_value();
      }
   } );
}

static void adjust_reward_balance(itemp_database& db, const account_object& a, const asset& delta )
{
   balance_processor bp(db);
   bp.adjust_reward_balance(a, delta);
}

static void adjust_supply(itemp_database& db, const asset& delta, bool adjust_vesting = false )
{
   balance_processor bp(db);
   bp.adjust_supply(delta, adjust_vesting);   
}

/**
 * This method updates total_reward_shares2 on DGPO, and children_rshares2 on comments, when a comment's rshares2 changes
 * from old_rshares2 to new_rshares2.  Maintaining invariants that children_rshares2 is the sum of all descendants' rshares2,
 * and dgpo.total_reward_shares2 is the total number of rshares2 outstanding.
 */
static void adjust_rshares2(itemp_database& db, const comment_object& c, fc::uint128_t old_rshares2, fc::uint128_t new_rshares2 )
{
   const auto& dgpo = db.get_dynamic_global_properties();
   db.modify( dgpo, [&]( dynamic_global_property_object& p )
   {
      p.total_reward_shares2 -= old_rshares2;
      p.total_reward_shares2 += new_rshares2;
   } );
}

static void validate_invariants(const itemp_database& db)
{
   invariant_validator validator(db);
   validator.validate();
}


/**
 * @brief Get the witness scheduled for block production in a slot.
 *
 * slot_num always corresponds to a time in the future.
 *
 * If slot_num == 1, returns the next scheduled witness.
 * If slot_num == 2, returns the next scheduled witness after
 * 1 block gap.
 *
 * Use the get_slot_time() and get_slot_at_time() functions
 * to convert between slot_num and timestamp.
 *
 * Passing slot_num == 0 returns STEEMIT_NULL_WITNESS
 */

static account_name_type get_scheduled_witness(const itemp_database& db, uint32_t slot_num )
{
   const dynamic_global_property_object& dpo = db.get_dynamic_global_properties();
   const witness_schedule_object& wso = db.get_witness_schedule_object();
   uint64_t current_aslot = dpo.current_aslot + slot_num;
   return wso.current_shuffled_witnesses[ current_aslot % wso.num_scheduled_witnesses ];
}

/**
 * @param to_account - the account to receive the new vesting shares
 * @param STEEM - STEEM to be converted to vesting shares
 */
static asset create_vesting(itemp_database& db, const account_object& to_account, asset steem, bool to_reward_balance )
{
   fund_processor fp(db);
   return fp.create_vesting(to_account, steem, to_reward_balance);
}

// not doing actual calc, just get data, should be renamed to get_discussion_payout_time
static const time_point_sec calculate_discussion_payout_time(const itemp_database& db, const comment_object& comment )
{
   if( db.has_hardfork( STEEMIT_HARDFORK_0_17 ) || comment.parent_author == STEEMIT_ROOT_POST_PARENT )
      return comment.cashout_time;
   else
      return db.get< comment_object >( comment.root_comment ).cashout_time;
}

static void cancel_order(itemp_database& db, const limit_order_object& order )
{
   db.adjust_balance( db.get_account(order.seller), order.amount_for_sale() );
   db.remove(order);
}

static bool apply_order(itemp_database& db, const limit_order_object& new_order_object )
{
   order_processor op(db);
   return op.apply_order(new_order_object);   
}

static fc::time_point_sec get_slot_time(const itemp_database& db, uint32_t slot_num)
{
   slot s(db);
   return s.get_slot_time(slot_num);
}

static uint32_t get_slot_at_time(const itemp_database& db,fc::time_point_sec when)
{
   slot s(db);
   return s.get_slot_at_time(when);
}

// This happens when two witness nodes are using same account
static void maybe_warn_multiple_production(const itemp_database& db, uint32_t height )
{
   auto blocks = db.fork_db().fetch_block_by_number( height );
   if( blocks.size() > 1 )
   {
      vector< std::pair< account_name_type, fc::time_point_sec > > witness_time_pairs;
      for( const auto& b : blocks )
      {
         witness_time_pairs.push_back( std::make_pair( b->data.witness, b->data.timestamp ) );
      }

      ilog( "Encountered block num collision at block ${n} due to a fork, witnesses are: ${w}", ("n", height)("w", witness_time_pairs) );
   }
   return;
}

static void adjust_liquidity_reward(itemp_database& db, const account_object& owner, const asset& volume, bool is_sdb )
{
   const auto& ridx = db.get_index< liquidity_reward_balance_index >().indices().get< by_owner >();
   auto itr = ridx.find( owner.id );
   if( itr != ridx.end() )
   {
      db.modify<liquidity_reward_balance_object>( *itr, [&]( liquidity_reward_balance_object& r )
      {
         if( db.head_block_time() - r.last_update >= STEEMIT_LIQUIDITY_TIMEOUT_SEC )
         {
            r.sbd_volume = 0;
            r.steem_volume = 0;
            r.weight = 0;
         }

         if( is_sdb )
            r.sbd_volume += volume.amount.value;
         else
            r.steem_volume += volume.amount.value;

         r.update_weight( db.has_hardfork( STEEMIT_HARDFORK_0_10 ) );
         r.last_update = db.head_block_time();
      } );
   }
   else
   {
      db.create<liquidity_reward_balance_object>( [&](liquidity_reward_balance_object& r )
      {
         r.owner = owner.id;
         if( is_sdb )
            r.sbd_volume = volume.amount.value;
         else
            r.steem_volume = volume.amount.value;

         r.update_weight( db.has_hardfork( STEEMIT_HARDFORK_0_9 ) );
         r.last_update = db.head_block_time();
      } );
   }
}

static share_type pay_reward_funds(itemp_database& db, share_type reward )
{
   const auto& reward_idx = db.get_index< reward_fund_index, by_id >();
   share_type used_rewards = 0;

   for( auto itr = reward_idx.begin(); itr != reward_idx.end(); ++itr )
   {
      // reward is a per block reward and the percents are 16-bit. This should never overflow
      auto r = ( reward * itr->percent_content_rewards ) / STEEMIT_100_PERCENT;

      db.modify( *itr, [&]( reward_fund_object& rfo )
      {
         rfo.reward_balance += asset( r, STEEM_SYMBOL );
      });

      used_rewards += r;

      // Sanity check to ensure we aren't printing more STEEM than has been allocated through inflation
      FC_ASSERT( used_rewards <= reward );
   }

   return used_rewards;
}

/** clears all vote records for a particular account but does not update the
 * witness vote totals.  Vote totals should be updated first via a call to
 * adjust_proxied_witness_votes( a, -a.witness_vote_weight() )
 */
static void clear_witness_votes(itemp_database& db, const account_object& a )
{
   const auto& vidx = db.get_index< witness_vote_index >().indices().get<by_account_witness>();
   auto itr = vidx.lower_bound( boost::make_tuple( a.id, witness_id_type() ) );
   while( itr != vidx.end() && itr->account == a.id )
   {
      const auto& current = *itr;
      ++itr;
      db.remove(current);
   }

   if( db.has_hardfork( STEEMIT_HARDFORK_0_19 ) )
      db.modify( a, [&](account_object& acc )
      {
         acc.witnesses_voted_for = 0;
      });
}

static asset get_curation_reward(const itemp_database& db)
{
   const auto& props = db.get_dynamic_global_properties();
   static_assert( STEEMIT_BLOCK_INTERVAL == 3, "this code assumes a 3-second time interval" );
   asset percent( protocol::calc_percent_reward_per_block< STEEMIT_CURATE_APR_PERCENT >( props.virtual_supply.amount ), STEEM_SYMBOL);
   return std::max( percent, STEEMIT_MIN_CURATE_REWARD );
}

static uint16_t get_curation_rewards_percent(const itemp_database& db, const comment_object& c )
{
   if( db.has_hardfork( STEEMIT_HARDFORK_0_17 ) )
      return db.get_reward_fund( c ).percent_curation_rewards;
   //else if( has_hardfork( STEEMIT_HARDFORK_0_8 ) )
   //   return STEEMIT_1_PERCENT * 25;
   else
      return STEEMIT_1_PERCENT * 50;
}

// TODO: REFACTORY: it's a get, should not update data inside
static asset get_producer_reward(itemp_database& db)
{
   const auto& props = db.get_dynamic_global_properties();
   static_assert( STEEMIT_BLOCK_INTERVAL == 3, "this code assumes a 3-second time interval" );
   asset percent( protocol::calc_percent_reward_per_block< STEEMIT_PRODUCER_APR_PERCENT >( props.virtual_supply.amount ), STEEM_SYMBOL);
   auto pay = std::max( percent, STEEMIT_MIN_PRODUCER_REWARD );
   const auto& witness_account = db.get_account( props.current_witness );

   /// pay witness in vesting shares
   if( props.head_block_number >= STEEMIT_START_MINER_VOTING_BLOCK || (witness_account.vesting_shares.amount.value == 0) ) {
      // const auto& witness_obj = get_witness( props.current_witness );
      const auto& producer_reward = create_vesting(db, witness_account, pay );
      db.push_virtual_operation( producer_reward_operation( witness_account.name, producer_reward ) );
   }
   else
   {
      db.modify( db.get_account( witness_account.name), [&]( account_object& a )
      {
         a.balance += pay;
      } );
   }

   return pay;
}

}}
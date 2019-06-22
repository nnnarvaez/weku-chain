#include <weku/chain/itemp_database.hpp>

namespace weku {namespace chain{

// This happens when two witness nodes are using same account
static void maybe_warn_multiple_production(itemp_database& db, uint32_t height )const
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
      const auto& producer_reward = db.create_vesting( witness_account, pay );
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
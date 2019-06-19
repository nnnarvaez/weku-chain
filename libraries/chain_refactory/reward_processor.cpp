#include <weku/chain/reward_processor.hpp>

namespace weku{namespace chain{

share_type reward_processor::pay_reward_funds( share_type reward )
{
   const auto& reward_idx = _db.get_index< reward_fund_index, by_id >();
   share_type used_rewards = 0;

   for( auto itr = reward_idx.begin(); itr != reward_idx.end(); ++itr )
   {
      // reward is a per block reward and the percents are 16-bit. This should never overflow
      auto r = ( reward * itr->percent_content_rewards ) / STEEMIT_100_PERCENT;

      _db.modify( *itr, [&]( reward_fund_object& rfo )
      {
         rfo.reward_balance += asset( r, STEEM_SYMBOL );
      });

      used_rewards += r;

      // Sanity check to ensure we aren't printing more STEEM than has been allocated through inflation
      FC_ASSERT( used_rewards <= reward );
   }

   return used_rewards;
}

asset reward_processor::get_liquidity_reward()const
{
   if( _db.has_hardfork( STEEMIT_HARDFORK_0_12 ) )
      return asset( 0, STEEM_SYMBOL );

   const auto& props = _db.get_dynamic_global_properties();
   static_assert( STEEMIT_LIQUIDITY_REWARD_PERIOD_SEC == 60*60, "this code assumes a 1 hour time interval" );
   asset percent( weku::protocol::calc_percent_reward_per_hour< STEEMIT_LIQUIDITY_APR_PERCENT >( props.virtual_supply.amount ), STEEM_SYMBOL );
   return std::max( percent, STEEMIT_MIN_LIQUIDITY_REWARD );
}

void reward_processor::pay_liquidity_reward()
{
   // #ifdef IS_TEST_NET
   // if( !liquidity_rewards_enabled )
   //    return;
   // #endif

   if( (_db.head_block_num() % STEEMIT_LIQUIDITY_REWARD_BLOCKS) == 0 )
   {
      auto reward = get_liquidity_reward();

      if( reward.amount == 0 )
         return;

      const auto& ridx = _db.get_index< liquidity_reward_balance_index >().indices().get< by_volume_weight >();
      auto itr = ridx.begin();
      if( itr != ridx.end() && itr->volume_weight() > 0 )
      {
         _db.adjust_supply( reward, true );
         _db.adjust_balance( _db.get(itr->owner), reward );
         _db.modify( *itr, [&]( liquidity_reward_balance_object& obj )
         {
            obj.steem_volume = 0;
            obj.sbd_volume   = 0;
            obj.last_update  = _db.head_block_time();
            obj.weight = 0;
         } );

         _db.push_virtual_operation( liquidity_reward_operation( _db.get(itr->owner).name, reward ) );
      }
   }
}

void reward_processor::adjust_liquidity_reward( const account_object& owner, const asset& volume, bool is_sdb )
{
   const auto& ridx = _db.get_index< liquidity_reward_balance_index >().indices().get< by_owner >();
   auto itr = ridx.find( owner.id );
   if( itr != ridx.end() )
   {
      _db.modify<liquidity_reward_balance_object>( *itr, [&]( liquidity_reward_balance_object& r )
      {
         if( _db.head_block_time() - r.last_update >= STEEMIT_LIQUIDITY_TIMEOUT_SEC )
         {
            r.sbd_volume = 0;
            r.steem_volume = 0;
            r.weight = 0;
         }

         if( is_sdb )
            r.sbd_volume += volume.amount.value;
         else
            r.steem_volume += volume.amount.value;

         r.update_weight( _db.has_hardfork( STEEMIT_HARDFORK_0_10 ) );
         r.last_update = _db.head_block_time();
      } );
   }
   else
   {
      _db.create<liquidity_reward_balance_object>( [&](liquidity_reward_balance_object& r )
      {
         r.owner = owner.id;
         if( is_sdb )
            r.sbd_volume = volume.amount.value;
         else
            r.steem_volume = volume.amount.value;

         r.update_weight( _db.has_hardfork( STEEMIT_HARDFORK_0_9 ) );
         r.last_update = _db.head_block_time();
      } );
   }
}

}}
#include <weku/chain/reward_processor.hpp>

namespace weku{namespace chain{



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
         adjust_supply(_db, reward, true );
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



}}
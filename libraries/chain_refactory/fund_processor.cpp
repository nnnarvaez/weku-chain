#include <wk/chain_refactory/fund_processor.hpp>

using namespace steemit::chain;

namespace wk{namespace chain{

/**
 *  Overall the network has an inflation rate of 102% of virtual steem per year
 *  90% of inflation is directed to vesting shares
 *  10% of inflation is directed to subjective proof of work voting
 *  1% of inflation is directed to liquidity providers
 *  1% of inflation is directed to block producers
 *
 *  This method pays out vesting and reward shares every block, and liquidity shares once per day.
 *  This method does not pay out witnesses.
 */
void fund_processor::process_funds()
{
   const auto& props = _db.get_dynamic_global_properties();
   const auto& wso = _db.get_witness_schedule_object();

    // since process_funds happens before process_hardfork, so block #1 will not have hardfork_0_16_551 (any hardfork)
   if( _db.has_hardfork( STEEMIT_HARDFORK_0_16) )
   {
      /**
       * At block 7,000,000 have a 9.5% instantaneous inflation rate, decreasing to 0.95% at a rate of 0.01%
       * every 250k blocks. This narrowing will take approximately 20.5 years and will complete on block 220,750,000
       */
      int64_t start_inflation_rate = int64_t( STEEMIT_INFLATION_RATE_START_PERCENT );
      int64_t inflation_rate_adjustment = int64_t( head_block_num() / STEEMIT_INFLATION_NARROWING_PERIOD );
      int64_t inflation_rate_floor = int64_t( STEEMIT_INFLATION_RATE_STOP_PERCENT );

      // below subtraction cannot underflow int64_t because inflation_rate_adjustment is <2^32
      int64_t current_inflation_rate = std::max( start_inflation_rate - inflation_rate_adjustment, inflation_rate_floor );

      auto new_steem = ( props.virtual_supply.amount * current_inflation_rate ) / ( int64_t( STEEMIT_100_PERCENT ) * int64_t( STEEMIT_BLOCKS_PER_YEAR ) );

      // hardfork 20 fixes the inflation rate at 5%
      if(_db.has_hardfork(STEEMIT_HARDFORK_0_20))
         new_steem = ( share_type(STEEMIT_INIT_SUPPLY) * int64_t(STEEMIT_INFLATION_RATE_PERCENT_0_20) ) / ( int64_t( STEEMIT_100_PERCENT ) * int64_t( STEEMIT_BLOCKS_PER_YEAR ) );

      auto content_reward = ( new_steem * STEEMIT_CONTENT_REWARD_PERCENT ) / STEEMIT_100_PERCENT;
      if( _db.has_hardfork( STEEMIT_HARDFORK_0_17 ) )
          // distribute the content_reward to all participants according to their percentage.
         content_reward = _db.pay_reward_funds( content_reward ); /// 75% to content creator
      auto vesting_reward = ( new_steem * STEEMIT_VESTING_FUND_PERCENT ) / STEEMIT_100_PERCENT; /// 15% to vesting fund
      auto witness_reward = new_steem - content_reward - vesting_reward; /// Remaining 10% to witness pay

      const auto& cwit = _db.get_witness( props.current_witness );
      witness_reward *= STEEMIT_MAX_WITNESSES;

      if( cwit.schedule == witness_object::timeshare )
         witness_reward *= wso.timeshare_weight;
      else if( cwit.schedule == witness_object::miner )
         witness_reward *= wso.miner_weight;
      else if( cwit.schedule == witness_object::top19 )
         witness_reward *= wso.top19_weight;
      else
         wlog( "Encountered unknown witness type for witness: ${w}", ("w", cwit.owner) );

      witness_reward /= wso.witness_pay_normalization_factor;

      // retally the new_steem
      new_steem = content_reward + vesting_reward + witness_reward;

      _db.modify( props, [&]( dynamic_global_property_object& p )
      {
          // as the total_vesting_fund_steem growing without growing total_vesting_shares,
          // then the price will gradually reduce, means 1 weku will buy less and less vesting.
          // note: price = total vesting / total vesting fund
          // basically, vesting_reward will benefit power up users by reducing the price.
         p.total_vesting_fund_steem += asset( vesting_reward, STEEM_SYMBOL );
         if( !has_hardfork( STEEMIT_HARDFORK_0_17 ) )
            p.total_reward_fund_steem  += asset( content_reward, STEEM_SYMBOL );
         p.current_supply           += asset( new_steem, STEEM_SYMBOL );
         p.virtual_supply           += asset( new_steem, STEEM_SYMBOL );
      });

      const auto& producer_reward = _db.create_vesting( _db.get_account( cwit.owner ), asset( witness_reward, STEEM_SYMBOL ) );
      _db.push_virtual_operation( producer_reward_operation( cwit.owner, producer_reward ) );

   }
   else
   {
      auto content_reward = _db.get_content_reward();
      auto curate_reward = _db.get_curation_reward();
      auto witness_pay = _db.get_producer_reward();
      auto vesting_reward = content_reward + curate_reward + witness_pay;

      content_reward = content_reward + curate_reward;

      if( props.head_block_number < STEEMIT_START_VESTING_BLOCK )
         vesting_reward.amount = 0;
      else
         vesting_reward.amount.value *= 9;

      _db.modify( props, [&]( dynamic_global_property_object& p )
      {
          p.total_vesting_fund_steem += vesting_reward;
          p.total_reward_fund_steem  += content_reward;
          p.current_supply += content_reward + witness_pay + vesting_reward;
          p.virtual_supply += content_reward + witness_pay + vesting_reward;
      } );
   }
}

}}
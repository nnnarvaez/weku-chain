#include <weku/chain/fund_processor.hpp>
#include <weku/chain/helpers.hpp>

namespace weku{namespace chain{

asset get_content_reward(const itemp_database& db)
{
   const auto& props = db.get_dynamic_global_properties();
   static_assert( STEEMIT_BLOCK_INTERVAL == 3, "this code assumes a 3-second time interval" );
   asset percent( protocol::calc_percent_reward_per_block< STEEMIT_CONTENT_APR_PERCENT >( props.virtual_supply.amount ), STEEM_SYMBOL );
   return std::max( percent, STEEMIT_MIN_CONTENT_REWARD );
}

asset fund_processor::create_vesting( const account_object& to_account, asset steem, bool to_reward_balance )
{
   try
   {
      const auto& cprops = _db.get_dynamic_global_properties();

      /**
       *  The ratio of total_vesting_shares / total_vesting_fund_steem should not
       *  change as the result of the user adding funds
       *
       *  V / C  = (V+Vn) / (C+Cn)
       *
       *  Simplifies to Vn = (V * Cn ) / C
       *
       *  If Cn equals o.amount, then we must solve for Vn to know how many new vesting shares
       *  the user should receive.
       *
       *  128 bit math is requred due to multiplying of 64 bit numbers. This is done in asset and price.
       */

      // The V/C ratio is not changed inside this function,
      // but it's being changed by the gradually reduced price in process_fund function
      // vesting_share_price means how many VESTS you can get per WEKU
      asset new_vesting = steem * ( to_reward_balance ? cprops.get_reward_vesting_share_price() : cprops.get_vesting_share_price() );

      _db.modify( to_account, [&]( account_object& to )
      {
         if( to_reward_balance )
         {
             // reward vesting balance and reward_vesting_steem are adding at same time?
            to.reward_vesting_balance += new_vesting;
            to.reward_vesting_steem += steem;
         }
         else
            to.vesting_shares += new_vesting;
      } );

      _db.modify( cprops, [&]( dynamic_global_property_object& props )
      {
         if( to_reward_balance )
         {
            props.pending_rewarded_vesting_shares += new_vesting;
            props.pending_rewarded_vesting_steem += steem;
         }
         else
         {
            props.total_vesting_fund_steem += steem;
            props.total_vesting_shares += new_vesting;
         }
      } );

      if( !to_reward_balance )
         _db.adjust_proxied_witness_votes( to_account, new_vesting.amount );

      return new_vesting;
   }
   FC_CAPTURE_AND_RETHROW( (to_account.name)(steem) )
}

/**
 *  Converts STEEM into sbd and adds it to to_account while reducing the STEEM supply
 *  by STEEM and increasing the sbd supply by the specified amount.
 */
std::pair< asset, asset > fund_processor::create_sbd( const account_object& to_account, asset steem, bool to_reward_balance )
{
   std::pair< asset, asset > assets( asset( 0, SBD_SYMBOL ), asset( 0, STEEM_SYMBOL ) );

   try
   {
      if( steem.amount == 0 )
         return assets;

      const auto& median_price = _db.get_feed_history().current_median_history;
      const auto& gpo = _db.get_dynamic_global_properties();

      if( !median_price.is_null() )
      {
          // for example: sbd_print_rate = 20%,  steem.amount = 100 WEKUK
          // then amount for to_sbd will be 20 , and amount for to_steem willbe 80
         auto to_sbd = ( gpo.sbd_print_rate * steem.amount ) / STEEMIT_100_PERCENT;
         auto to_steem = steem.amount - to_sbd;

         // to_sbd is an amount of WEKU need to be convert to SBD
         // median_price is SBD/WEKU, means how much SBD per WEKU.
         // so the UNIT of variable sbd here will be SBD
         auto sbd = asset( to_sbd, STEEM_SYMBOL ) * median_price;

         if( to_reward_balance )
         {
            adjust_reward_balance(_db, to_account, sbd );
            adjust_reward_balance(_db, to_account, asset( to_steem, STEEM_SYMBOL ) );
         }
         else
         {
            _db.adjust_balance( to_account, sbd );
            _db.adjust_balance( to_account, asset( to_steem, STEEM_SYMBOL ) );
         }

         adjust_supply(_db, asset( -to_sbd, STEEM_SYMBOL ) );
         adjust_supply(_db, sbd );
         assets.first = sbd;
         assets.second = to_steem;
      }
      else
      {
          // if no median_price availabe, it will just add weku directly without SBD.
         _db.adjust_balance( to_account, steem );
         assets.second = steem;
      }
   }
   FC_CAPTURE_LOG_AND_RETHROW( (to_account.name)(steem) )

   return assets;
}

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
      int64_t inflation_rate_adjustment = int64_t( _db.head_block_num() / STEEMIT_INFLATION_NARROWING_PERIOD );
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
         content_reward = pay_reward_funds(_db, content_reward ); /// 75% to content creator
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
         if( !_db.has_hardfork( STEEMIT_HARDFORK_0_17 ) )
            p.total_reward_fund_steem  += asset( content_reward, STEEM_SYMBOL );
         p.current_supply           += asset( new_steem, STEEM_SYMBOL );
         p.virtual_supply           += asset( new_steem, STEEM_SYMBOL );
      });

      const auto& producer_reward = create_vesting(_db, _db.get_account( cwit.owner ), asset( witness_reward, STEEM_SYMBOL ) );
      _db.push_virtual_operation( producer_reward_operation( cwit.owner, producer_reward ) );

   }
   else
   {
      auto content_reward = get_content_reward(_db);
      auto curate_reward = get_curation_reward(_db);
      auto witness_pay = get_producer_reward(_db);
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
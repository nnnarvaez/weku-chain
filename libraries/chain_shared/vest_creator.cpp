#include <weku/chain/vest_creator.hpp>

namespace weku{namespace chain{
asset vest_creator::create_vesting( const account_object& to_account, asset steem, bool to_reward_balance )
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

}}
#include <weku/chain/null_account_cleaner.hpp>

namespace weku{namespace chain{

// get called for every block creation.
void null_account_cleaner::clear_null_account_balance()
{
   if( !_db.has_hardfork( STEEMIT_HARDFORK_0_14 ) ) return;

   const auto& null_account = _db.get_account( STEEMIT_NULL_ACCOUNT );
   asset total_steem( 0, STEEM_SYMBOL );
   asset total_sbd( 0, SBD_SYMBOL );

   if( null_account.balance.amount > 0 )
   {
      total_steem += null_account.balance;
      _db.adjust_balance( null_account, -null_account.balance );
   }

   if( null_account.savings_balance.amount > 0 )
   {
      total_steem += null_account.savings_balance;
      _db.adjust_savings_balance( null_account, -null_account.savings_balance );
   }

   if( null_account.sbd_balance.amount > 0 )
   {
      total_sbd += null_account.sbd_balance;
      _db.adjust_balance( null_account, -null_account.sbd_balance );
   }

   if( null_account.savings_sbd_balance.amount > 0 )
   {
      total_sbd += null_account.savings_sbd_balance;
      _db.adjust_savings_balance( null_account, -null_account.savings_sbd_balance );
   }

   if( null_account.vesting_shares.amount > 0 )
   {
      const auto& gpo = _db.get_dynamic_global_properties();
      // TODO: need to confirm if below calc is correct or not.
      // QUESTION: should below calc using divide instead of multiply?
      auto converted_steem = null_account.vesting_shares * gpo.get_vesting_share_price();

      _db.modify( gpo, [&]( dynamic_global_property_object& g )
      {
         g.total_vesting_shares -= null_account.vesting_shares;
         g.total_vesting_fund_steem -= converted_steem;
      });

      _db.modify( null_account, [&]( account_object& a )
      {
         a.vesting_shares.amount = 0;
      });

      total_steem += converted_steem;
   }

   if( null_account.reward_steem_balance.amount > 0 )
   {
      total_steem += null_account.reward_steem_balance;
      adjust_reward_balance(_db, null_account, -null_account.reward_steem_balance );
   }

   if( null_account.reward_sbd_balance.amount > 0 )
   {
      total_sbd += null_account.reward_sbd_balance;
      adjust_reward_balance(_db, null_account, -null_account.reward_sbd_balance );
   }

   if( null_account.reward_vesting_balance.amount > 0 )
   {
      const auto& gpo = _db.get_dynamic_global_properties();

      total_steem += null_account.reward_vesting_steem;

      _db.modify( gpo, [&]( dynamic_global_property_object& g )
      {
         g.pending_rewarded_vesting_shares -= null_account.reward_vesting_balance;
         g.pending_rewarded_vesting_steem -= null_account.reward_vesting_steem;
      });

      _db.modify( null_account, [&]( account_object& a )
      {
         a.reward_vesting_steem.amount = 0;
         a.reward_vesting_balance.amount = 0;
      });
   }

   if( total_steem.amount > 0 )
      adjust_supply(_db, -total_steem );

   if( total_sbd.amount > 0 )
      adjust_supply(_db, -total_sbd );
}


}}
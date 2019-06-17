#include <steemit/chain/invariant_validator.hpp>

void invariant_validator::validate(){
    try
   {
      const auto& account_idx = _db.get_index<account_index>().indices().get<by_name>();
      asset total_supply = asset( 0, STEEM_SYMBOL );
      asset total_sbd = asset( 0, SBD_SYMBOL );
      asset total_vesting = asset( 0, VESTS_SYMBOL );
      asset pending_vesting_steem = asset( 0, STEEM_SYMBOL );
      share_type total_vsf_votes = share_type( 0 );

      auto gpo = _db.get_dynamic_global_properties();

      /// verify no witness has too many votes
      const auto& witness_idx = _db.get_index< witness_index >().indices();
      for( auto itr = witness_idx.begin(); itr != witness_idx.end(); ++itr )
         FC_ASSERT( itr->votes <= gpo.total_vesting_shares.amount, "", ("itr",*itr) );

      for( auto itr = account_idx.begin(); itr != account_idx.end(); ++itr )
      {
         total_supply += itr->balance;
         total_supply += itr->savings_balance;
         total_supply += itr->reward_steem_balance;
         total_sbd += itr->sbd_balance;
         total_sbd += itr->savings_sbd_balance;
         total_sbd += itr->reward_sbd_balance;
         total_vesting += itr->vesting_shares;
         total_vesting += itr->reward_vesting_balance;
         pending_vesting_steem += itr->reward_vesting_steem;
         // QUESTION: what is total_vsf_votes stands for?
         total_vsf_votes += ( itr->proxy == STEEMIT_PROXY_TO_SELF_ACCOUNT ?
                                 itr->witness_vote_weight() :
                                 ( STEEMIT_MAX_PROXY_RECURSION_DEPTH > 0 ?
                                      itr->proxied_vsf_votes[STEEMIT_MAX_PROXY_RECURSION_DEPTH - 1] :
                                      itr->vesting_shares.amount ) );
      }

      const auto& convert_request_idx = _db.get_index< convert_request_index >().indices();

      for( auto itr = convert_request_idx.begin(); itr != convert_request_idx.end(); ++itr )
      {
         if( itr->amount.symbol == STEEM_SYMBOL )
            total_supply += itr->amount;
         else if( itr->amount.symbol == SBD_SYMBOL )
            total_sbd += itr->amount;
         else
            FC_ASSERT( false, "Encountered illegal symbol in convert_request_object" );
      }

      const auto& limit_order_idx = _db.get_index< limit_order_index >().indices();

      for( auto itr = limit_order_idx.begin(); itr != limit_order_idx.end(); ++itr )
      {
         if( itr->sell_price.base.symbol == STEEM_SYMBOL )
         {
            total_supply += asset( itr->for_sale, STEEM_SYMBOL );
         }
         else if ( itr->sell_price.base.symbol == SBD_SYMBOL )
         {
            total_sbd += asset( itr->for_sale, SBD_SYMBOL );
         }
      }

      const auto& escrow_idx = _db.get_index< escrow_index >().indices().get< by_id >();

      for( auto itr = escrow_idx.begin(); itr != escrow_idx.end(); ++itr )
      {
         total_supply += itr->steem_balance;
         total_sbd += itr->sbd_balance;

         if( itr->pending_fee.symbol == STEEM_SYMBOL )
            total_supply += itr->pending_fee;
         else if( itr->pending_fee.symbol == SBD_SYMBOL )
            total_sbd += itr->pending_fee;
         else
            FC_ASSERT( false, "found escrow pending fee that is not SBD or STEEM" );
      }

      const auto& savings_withdraw_idx = _db.get_index< savings_withdraw_index >().indices().get< by_id >();

      for( auto itr = savings_withdraw_idx.begin(); itr != savings_withdraw_idx.end(); ++itr )
      {
         if( itr->amount.symbol == STEEM_SYMBOL )
            total_supply += itr->amount;
         else if( itr->amount.symbol == SBD_SYMBOL )
            total_sbd += itr->amount;
         else
            FC_ASSERT( false, "found savings withdraw that is not SBD or STEEM" );
      }
      fc::uint128_t total_rshares2;

      const auto& comment_idx = _db.get_index< comment_index >().indices();

      for( auto itr = comment_idx.begin(); itr != comment_idx.end(); ++itr )
      {
         if( itr->net_rshares.value > 0 )
         {
            auto delta = util::evaluate_reward_curve( itr->net_rshares.value );
            total_rshares2 += delta;
         }
      }

      const auto& reward_idx = _db.get_index< reward_fund_index, by_id >();

      for( auto itr = reward_idx.begin(); itr != reward_idx.end(); ++itr )
      {
         total_supply += itr->reward_balance;
      }

      total_supply += gpo.total_vesting_fund_steem + gpo.total_reward_fund_steem + gpo.pending_rewarded_vesting_steem;    

      FC_ASSERT( gpo.current_supply == total_supply, "", ("gpo.current_supply",gpo.current_supply)("total_supply",total_supply) );
      FC_ASSERT( gpo.current_sbd_supply == total_sbd, "", ("gpo.current_sbd_supply",gpo.current_sbd_supply)("total_sbd",total_sbd) );
      FC_ASSERT( gpo.total_vesting_shares + gpo.pending_rewarded_vesting_shares == total_vesting, "", ("gpo.total_vesting_shares",gpo.total_vesting_shares)("total_vesting",total_vesting) );
      FC_ASSERT( gpo.total_vesting_shares.amount == total_vsf_votes, "", ("total_vesting_shares AMOUNT",gpo.total_vesting_shares.amount)("total_vsf_votes",total_vsf_votes) );
      FC_ASSERT( gpo.pending_rewarded_vesting_steem == pending_vesting_steem, "", ("pending_rewarded_vesting_steem",gpo.pending_rewarded_vesting_steem)("pending_vesting_steem", pending_vesting_steem));

      FC_ASSERT( gpo.virtual_supply >= gpo.current_supply );
      if ( !get_feed_history().current_median_history.is_null() )
      {
         FC_ASSERT( gpo.current_sbd_supply * get_feed_history().current_median_history + gpo.current_supply
            == gpo.virtual_supply, "", ("gpo.current_sbd_supply",gpo.current_sbd_supply)("get_feed_history().current_median_history",get_feed_history().current_median_history)("gpo.current_supply",gpo.current_supply)("gpo.virtual_supply",gpo.virtual_supply) );
      }
   }
   FC_CAPTURE_LOG_AND_RETHROW( (head_block_num()) );
}
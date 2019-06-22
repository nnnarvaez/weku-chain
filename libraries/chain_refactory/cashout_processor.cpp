#include <weku/chain/cashout_processor.hpp>
#include <weku/chain/fund_processor.hpp>
#include <weku/chain/helpers.hpp>

#include <vector>

namespace weku{namespace chain{

using fc::uint128_t;

/**
 *  This method will iterate through all comment_vote_objects and give them
 *  (max_rewards * weight) / c.total_vote_weight.
 *
 *  @returns unclaimed rewards.
 */
share_type pay_curators(itemp_database& db, const comment_object& c, share_type& max_rewards )
{
   try
   {
      uint128_t total_weight( c.total_vote_weight );
      //edump( (total_weight)(max_rewards) );
      share_type unclaimed_rewards = max_rewards;

      if( !c.allow_curation_rewards )
      {
         unclaimed_rewards = 0;
         max_rewards = 0;
      }
      else if( c.total_vote_weight > 0 )
      {
         const auto& cvidx = db.get_index<comment_vote_index>().indices().get<by_comment_weight_voter>();
         auto itr = cvidx.lower_bound( c.id );
         while( itr != cvidx.end() && itr->comment == c.id )
         {
            uint128_t weight( itr->weight );
            auto claim = ( ( max_rewards.value * weight ) / total_weight ).to_uint64();
            if( claim > 0 ) // min_amt is non-zero satoshis
            {
               unclaimed_rewards -= claim;
               const auto& voter = db.get(itr->voter);
               auto reward = create_vesting(_db, voter, asset( claim, STEEM_SYMBOL ), db.has_hardfork( STEEMIT_HARDFORK_0_17 ) );

               db.push_virtual_operation( curation_reward_operation( voter.name, reward, c.author, to_string( c.permlink ) ) );

               #ifndef IS_LOW_MEM
               db.modify( voter, [&]( account_object& a )
               {
                  a.curation_rewards += claim;
               });
               #endif
            }
            ++itr;
         }
      }
      max_rewards -= unclaimed_rewards;

      return unclaimed_rewards;
   } FC_CAPTURE_AND_RETHROW()
}

void adjust_total_payout(itemp_database& db, const comment_object& cur, const asset& sbd_created, const asset& curator_sbd_value, const asset& beneficiary_value )
{
   db.modify( cur, [&]( comment_object& c )
   {
      if( c.total_payout_value.symbol == sbd_created.symbol )
         c.total_payout_value += sbd_created;
         c.curator_payout_value += curator_sbd_value;
         c.beneficiary_payout_value += beneficiary_value;
   } );
   /// TODO: potentially modify author's total payout numbers as well
}

void cashout_processor::fill_comment_reward_context_local_state( util::comment_reward_context& ctx, const comment_object& comment )
{
   ctx.rshares = comment.net_rshares;
   ctx.reward_weight = comment.reward_weight;
   ctx.max_sbd = comment.max_accepted_payout;
}

share_type cashout_processor::cashout_comment_helper( weku::chain::util::comment_reward_context& ctx, const comment_object& comment )
{
   try
   {
      share_type claimed_reward = 0;

      if( comment.net_rshares > 0 )
      {
         fill_comment_reward_context_local_state( ctx, comment );

         if( _db.has_hardfork( STEEMIT_HARDFORK_0_17 ) )
         {
            const auto rf = _db.get_reward_fund( comment );
            ctx.reward_curve = rf.author_reward_curve;
            ctx.content_constant = rf.content_constant;
         }

         const share_type reward = util::get_rshare_reward( ctx );
         uint128_t reward_tokens = uint128_t( reward.value );

         if( reward_tokens > 0 )
         {
            share_type curation_tokens = ( ( reward_tokens * get_curation_rewards_percent(_db, comment ) ) / STEEMIT_100_PERCENT ).to_uint64();
            share_type author_tokens = reward_tokens.to_uint64() - curation_tokens;

            author_tokens += pay_curators(_db, comment, curation_tokens );
            share_type total_beneficiary = 0;
            claimed_reward = author_tokens + curation_tokens;

            for( auto& b : comment.beneficiaries )
            {
               auto benefactor_tokens = ( author_tokens * b.weight ) / STEEMIT_100_PERCENT;
               auto vest_created = create_vesting(_db, _db.get_account( b.account ), benefactor_tokens, _db.has_hardfork( STEEMIT_HARDFORK_0_17 ) );
               _db.push_virtual_operation( comment_benefactor_reward_operation( b.account, comment.author, to_string( comment.permlink ), vest_created ) );
               total_beneficiary += benefactor_tokens;
            }

            author_tokens -= total_beneficiary;

            auto sbd_steem     = ( author_tokens * comment.percent_steem_dollars ) / ( 2 * STEEMIT_100_PERCENT ) ;
            auto vesting_steem = author_tokens - sbd_steem;

            const auto& author = _db.get_account( comment.author );
            auto vest_created = create_vesting(_db, author, vesting_steem, _db.has_hardfork( STEEMIT_HARDFORK_0_17 ) );
            
            fund_processor fpr(_db);
            auto sbd_payout = fpr.create_sbd( author, sbd_steem, _db.has_hardfork( STEEMIT_HARDFORK_0_17 ) );

            adjust_total_payout(_db, comment, sbd_payout.first + 
                to_sbd(_db, sbd_payout.second + asset( vesting_steem, STEEM_SYMBOL ) ), 
                to_sbd(_db, asset( curation_tokens, STEEM_SYMBOL ) ), 
                to_sbd(_db, asset( total_beneficiary, STEEM_SYMBOL ) ) );

            _db.push_virtual_operation( author_reward_operation( comment.author, to_string( comment.permlink ), 
                sbd_payout.first, sbd_payout.second, vest_created ) );
            _db.push_virtual_operation( comment_reward_operation( comment.author, to_string( comment.permlink ), 
                to_sbd(_db, asset( claimed_reward, STEEM_SYMBOL ) ) ) );

            #ifndef IS_LOW_MEM
            _db.modify( comment, [&]( comment_object& c )
            {
                c.author_rewards += author_tokens;
            });

            _db.modify( _db.get_account( comment.author ), [&]( account_object& a )
            {
                a.posting_rewards += author_tokens;
            });
            #endif

         }

         if( !_db.has_hardfork( STEEMIT_HARDFORK_0_17 ) )
            adjust_rshares2(_db, comment, util::evaluate_reward_curve( comment.net_rshares.value ), 0 );
      }

      _db.modify( comment, [&]( comment_object& c )
      {
         /**
         * A payout is only made for positive rshares, negative rshares hang around
         * for the next time this post might get an upvote.
         */
         if( c.net_rshares > 0 )
            c.net_rshares = 0;
         c.children_abs_rshares = 0;
         c.abs_rshares  = 0;
         c.vote_rshares = 0;
         c.total_vote_weight = 0;
         c.max_cashout_time = fc::time_point_sec::maximum();

         if( _db.has_hardfork( STEEMIT_HARDFORK_0_17 ) )
         {
            c.cashout_time = fc::time_point_sec::maximum();
         }
         else if( c.parent_author == STEEMIT_ROOT_POST_PARENT )
         {
            if( _db.has_hardfork( STEEMIT_HARDFORK_0_12 ) && c.last_payout == fc::time_point_sec::min() )
               c.cashout_time = _db.head_block_time() + STEEMIT_SECOND_CASHOUT_WINDOW;
            else
               c.cashout_time = fc::time_point_sec::maximum();
         }

         c.last_payout = _db.head_block_time();
      } );

      _db.push_virtual_operation( comment_payout_update_operation( comment.author, to_string( comment.permlink ) ) );

      const auto& vote_idx = _db.get_index< comment_vote_index >().indices().get< by_comment_voter >();
      auto vote_itr = vote_idx.lower_bound( comment.id );
      while( vote_itr != vote_idx.end() && vote_itr->comment == comment.id )
      {
         const auto& cur_vote = *vote_itr;
         ++vote_itr;
         if( !_db.has_hardfork( STEEMIT_HARDFORK_0_12 ) || calculate_discussion_payout_time(_db, comment ) != fc::time_point_sec::maximum() )
         {
            _db.modify( cur_vote, [&]( comment_vote_object& cvo )
            {
               cvo.num_changes = -1;
            });
         }
         else
         {
            #ifdef CLEAR_VOTES
            _db.remove( cur_vote );
            #endif
         }
      }

      return claimed_reward;
   } FC_CAPTURE_AND_RETHROW( (comment) )
}

void cashout_processor::process_comment_cashout()
{
   /// don't allow any content to get paid out until the website is ready to launch
   /// and people have had a week to start posting.  The first cashout will be the biggest because it
   /// will represent 2+ months of rewards.
   if( !_db.has_hardfork( STEEMIT_FIRST_CASHOUT_TIME ) )
      return;

   const auto& gpo = _db.get_dynamic_global_properties();
   util::comment_reward_context ctx;
   ctx.current_steem_price = _db.get_feed_history().current_median_history;

   std::vector< reward_fund_context > funds;
   std::vector< share_type > steem_awarded;
   const auto& reward_idx = _db.get_index< reward_fund_index, by_id >();

   // Decay recent rshares of each fund
   for( auto itr = reward_idx.begin(); itr != reward_idx.end(); ++itr )
   {
      // Add all reward funds to the local cache and decay their recent rshares
      _db.modify( *itr, [&]( reward_fund_object& rfo )
      {
         fc::microseconds decay_rate;

         if( _db.has_hardfork( STEEMIT_HARDFORK_0_19 ) )
            decay_rate = STEEMIT_RECENT_RSHARES_DECAY_RATE_HF19;
         else
            decay_rate = STEEMIT_RECENT_RSHARES_DECAY_RATE_HF17;

         rfo.recent_claims -= ( rfo.recent_claims * ( _db.head_block_time() - rfo.last_update ).to_seconds() ) / decay_rate.to_seconds();
         rfo.last_update = _db.head_block_time();
      });

      reward_fund_context rf_ctx;
      rf_ctx.recent_claims = itr->recent_claims;
      rf_ctx.reward_balance = itr->reward_balance;

      // The index is by ID, so the ID should be the current size of the vector (0, 1, 2, etc...)
      assert( funds.size() == size_t( itr->id._id ) );

      funds.push_back( rf_ctx );
   }

   const auto& cidx        = _db.get_index< comment_index >().indices().get< by_cashout_time >();
   const auto& com_by_root = _db.get_index< comment_index >().indices().get< by_root >();

   auto current = cidx.begin();
   //  add all rshares about to be cashed out to the reward funds. This ensures equal satoshi per rshare payment
   if( _db.has_hardfork( STEEMIT_HARDFORK_0_17 ) )
   {
      while( current != cidx.end() && current->cashout_time <= _db.head_block_time() )
      {
         if( current->net_rshares > 0 )
         {
            const auto& rf = _db.get_reward_fund( *current );
            funds[ rf.id._id ].recent_claims += util::evaluate_reward_curve( current->net_rshares.value, rf.author_reward_curve, rf.content_constant );
         }

         ++current;
      }

      current = cidx.begin();
   }

   /*
    * Payout all comments
    *
    * Each payout follows a similar pattern, but for a different reason.
    * Cashout comment helper does not know about the reward fund it is paying from.
    * The helper only does token allocation based on curation rewards and the SBD
    * global %, etc.
    *
    * Each context is used by get_rshare_reward to determine what part of each budget
    * the comment is entitled to. Prior to hardfork 17, all payouts are done against
    * the global state updated each payout. After the hardfork, each payout is done
    * against a reward fund state that is snapshotted before all payouts in the block.
    */
   while( current != cidx.end() && current->cashout_time <= _db.head_block_time() )
   {
      if( _db.has_hardfork( STEEMIT_HARDFORK_0_17 ) )
      {
         auto fund_id = _db.get_reward_fund( *current ).id._id;
         ctx.total_reward_shares2 = funds[ fund_id ].recent_claims;
         ctx.total_reward_fund_steem = funds[ fund_id ].reward_balance;
         funds[ fund_id ].steem_awarded += cashout_comment_helper( ctx, *current );
      }
      else
      {
         auto itr = com_by_root.lower_bound( current->root_comment );
         while( itr != com_by_root.end() && itr->root_comment == current->root_comment )
         {
            const auto& comment = *itr; ++itr;
            ctx.total_reward_shares2 = gpo.total_reward_shares2;
            ctx.total_reward_fund_steem = gpo.total_reward_fund_steem;

            auto reward = cashout_comment_helper( ctx, comment );

            if( reward > 0 )
            {
               _db.modify( _db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& p )
               {
                  p.total_reward_fund_steem.amount -= reward;
               });
            }
         }
      }

      current = cidx.begin();
   }

   // Write the cached fund state back to the database
   if( funds.size() )
   {
      for( size_t i = 0; i < funds.size(); i++ )
      {
         _db.modify( _db.get< reward_fund_object, by_id >( reward_fund_id_type( i ) ), [&]( reward_fund_object& rfo )
         {
            rfo.recent_claims = funds[ i ].recent_claims;
            rfo.reward_balance -= funds[ i ].steem_awarded;
         });
      }
   }
}

}}
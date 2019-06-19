#pragma once

#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{

struct reward_fund_context
{
   fc::uint128_t   recent_claims = 0;
   asset       reward_balance = asset( 0, STEEM_SYMBOL );
   share_type  steem_awarded = 0;
};

class cashout_processor{
    public:
        cashout_processor(itemp_database& db):_db(db){}
        void fill_comment_reward_context_local_state( util::comment_reward_context& ctx, const comment_object& comment );
        share_type cashout_comment_helper( weku::chain::util::comment_reward_context& ctx, const comment_object& comment );
        void process_comment_cashout();
    private:
        itemp_database& _db;
};

}}
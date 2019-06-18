#pragma once

#include <wk/chain_refactory/itemp_database.hpp>

namespace wk{namespace chain{

class cashout_processor{
    public:
        cashout_processor(itemp_database& db):_db(db){}
        share_type cashout_comment_helper( util::comment_reward_context& ctx, const comment_object& comment );
        void process_comment_cashout();
    private:
        itemp_database& _db;
};

}}
#pragma once

#include <wk/chain/itemp_database.hpp>

namespace wk{namespace chain{

class reward_processor{
    public:
        reward_processor(itemp_database& db):_db(db){}

        void adjust_liquidity_reward( const account_object& owner, const asset& volume, bool is_sdb );
    private:
        item_database& _db;
};

}}
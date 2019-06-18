#pragma once

#include <wk/chain_refactory/itemp_database.hpp>

namespace wk{namespace chain{

class reward_processor{
    public:
        reward_processor(itemp_database& db):_db(db){}

        void adjust_liquidity_reward( const account_object& owner, const asset& volume, bool is_sdb );
    private:
        itemp_database& _db;
};

}}
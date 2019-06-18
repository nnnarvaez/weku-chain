#pragma once

#include <wk/chain_refactory/itemp_database.hpp>

namespace wk{namespace chain{

class reward_processor{
    public:
        reward_processor(itemp_database& db):_db(db){}
        asset get_liquidity_reward()const;
        share_type pay_reward_funds( share_type reward );
        void pay_liquidity_reward();
        void adjust_liquidity_reward( const account_object& owner, const asset& volume, bool is_sdb );
    private:
        itemp_database& _db;
};

}}
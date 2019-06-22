#pragma once

#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{

class reward_processor{
    public:
        reward_processor(itemp_database& db):_db(db){}
        asset get_liquidity_reward()const;
        
        void pay_liquidity_reward();
        void adjust_liquidity_reward( const account_object& owner, const asset& volume, bool is_sdb );
    private:
        itemp_database& _db;
};

}}
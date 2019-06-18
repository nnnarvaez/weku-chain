#pragma once

#include <wk/chain_refactory/itemp_database.hpp>

namespace wk{namespace chain{

class balance_processor{
    public:
        balance_processor(itemp_database& db):_db(db){}
        void adjust_reward_balance( const account_object& a, const asset& delta );
        void adjust_savings_balance( const account_object& a, const asset& delta );
        void adjust_balance( const account_object& a, const asset& delta );
    private:
        itemp_database& _db;
};

}}
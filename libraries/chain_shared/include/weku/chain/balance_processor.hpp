#pragma once

#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{

class balance_processor{
    public:
        balance_processor(itemp_database& db):_db(db){}
        void adjust_supply( const asset& delta, bool adjust_vesting = false );
        void adjust_reward_balance( const account_object& a, const asset& delta );
        void adjust_savings_balance( const account_object& a, const asset& delta );
        void adjust_balance( const account_object& a, const asset& delta );
    private:
        itemp_database& _db;
};

}}
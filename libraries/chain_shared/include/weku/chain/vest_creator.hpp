#pragma once
#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{

class vest_creator{
    public:
    vest_creator(itemp_database& db):_db(db){}
    asset create_vesting( const account_object& to_account, asset steem, bool to_reward_balance=false );
    private:
    itemp_database& _db;
};

}}
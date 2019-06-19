#pragma once

#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{

class vest_withdraw_processor{
    public:
        vest_withdraw_processor(itemp_database& db):_db(db){}
        void process_vesting_withdrawals();
    private:
        itemp_database& _db;
};

}}
#pragma once

#include <wk/chain_refactory/itemp_database.hpp>

namespace wk{namespace chain{

class null_account_cleaner{
    public:
        null_account_cleaner(itemp_database& db):_db(db){}
        
        void clear_null_account_balance();
    private:
        itemp_database& _db;
};  

}}


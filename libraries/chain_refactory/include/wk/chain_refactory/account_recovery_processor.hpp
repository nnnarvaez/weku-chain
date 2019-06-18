#pragma once

#include <wk/chain_refactory/itemp_database.hpp>

namespace wk{namespace chain{

class account_recovery_processor{
    public:
        account_recovery_processor(itemp_database& db):_db(db){}
        void account_recovery_processing();
    private:
        itemp_database& _db;
};

}}
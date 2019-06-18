#pragma once

#include <wk/chain_refactory/itemp_database.hpp>

namespace wk{namespace chain{

class conversion_processor{
    public:
        conversion_processor(itemp_database& db):_db(db){}
        
        void process_conversions();
    private:
        itemp_database& _db;
};  

}}


#pragma once

#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{

class conversion_processor{
    public:
        conversion_processor(itemp_database& db):_db(db){}
        
        void process_conversions();
    private:
        itemp_database& _db;
};  

}}


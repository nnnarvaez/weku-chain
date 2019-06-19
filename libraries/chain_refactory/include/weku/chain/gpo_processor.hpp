#pragma once

#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{

class gpo_processor{
    public:
        gpo_processor(itemp_database& db):_db(db){}
        void update_global_dynamic_data( const signed_block& b );
    private:
        itemp_database& _db;
};

}}
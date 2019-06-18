#pragma once

#include <wk/chain_refactory/itemp_database.hpp>

namespace wk{namespace chain{

class median_feed_updator{
    public:
        median_feed_updator(itemp_database& db):_db(db){}
        
        void update_median_feed();
    private:
        itemp_database& _db;
};  

}}


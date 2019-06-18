#pragma once

#include <wk/chain_refactory/itemp_database.hpp>

namespace wk{namespace chain{

class order_processor{
    public:
        virtual bool apply_order( const limit_order_object& new_order_object );
        bool fill_order( const limit_order_object& order, const asset& pays, const asset& receives );
        void clear_expired_orders();
        int match( const limit_order_object& bid, 
            const limit_order_object& ask, 
            const price& trade_price );
    private:
        itemp_database& _db;
};  

}}


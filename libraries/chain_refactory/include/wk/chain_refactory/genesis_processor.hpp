#pragma once

#include <wk/chain_refactory/itemp_database.hpp>

namespace wk{namespace chain{

class genesis_processor{
    public:
        genesis_processor(itemp_database& db):_db(db){}
        void init_genesis(uint64_t initial_supply = STEEMIT_INIT_SUPPLY );
    private:
        itemp_database& _db;
};

}}
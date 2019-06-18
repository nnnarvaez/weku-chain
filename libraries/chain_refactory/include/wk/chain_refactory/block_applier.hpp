#pragma once

#include <wk/chain_refactory/itemp_database.hpp>

namespace wk{namespace chain{

class block_applier{
    public:
        block_applier(itemp_database& db):_db(db){}
        void update_signing_witness(const witness_object& signing_witness, const signed_block& new_block);
        void update_last_irreversible_block();
    private:
        itemp_database& _db;
};

}}
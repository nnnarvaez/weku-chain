#pragma once

#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{

class block_applier{
    public:
        block_applier(itemp_database& db):_db(db){}
        void update_signing_witness(const witness_object& signing_witness, const signed_block& new_block);
        void update_last_irreversible_block();
        void expire_escrow_ratification();
        void process_decline_voting_rights();
        void clear_expired_transactions();
        void clear_expired_delegations();
        void clear_expired_orders();
        void create_block_summary(const signed_block& next_block);
        void update_global_dynamic_data( const signed_block& b );
        void update_virtual_supply();
    private:
        itemp_database& _db;
};

}}
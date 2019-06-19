#include <weku/chain/genesis_processor.hpp>

namespace weku{namespace chain{

void genesis_processor::init_genesis( uint64_t init_supply )
{
    try
    {
        struct auth_inhibitor
        {
            public:
                auth_inhibitor(itemp_database& db) : xdb(db), old_flags(db.node_properties().skip_flags)
                    { xdb.node_properties().skip_flags |= skip_authority_check; }
                ~auth_inhibitor()
                    { xdb.node_properties().skip_flags = old_flags; }
            private:
                itemp_database& xdb;
                uint32_t old_flags;
        } inhibitor(_db);

        // Create blockchain accounts
        public_key_type init_public_key(STEEMIT_INIT_PUBLIC_KEY);

        _db.create< account_object >( [&]( account_object& a )
        {
            a.name = STEEMIT_MINER_ACCOUNT;
        } );
        
        _db.create< account_authority_object >( [&]( account_authority_object& auth )
        {
            auth.account = STEEMIT_MINER_ACCOUNT;
            auth.owner.weight_threshold = 1;
            auth.active.weight_threshold = 1;
        });

        _db.create< account_object >( [&]( account_object& a )
        {
            a.name = STEEMIT_NULL_ACCOUNT;
        } );
        
        _db.create< account_authority_object >( [&]( account_authority_object& auth )
        {
            auth.account = STEEMIT_NULL_ACCOUNT;
            auth.owner.weight_threshold = 1;
            auth.active.weight_threshold = 1;
        });

        _db.create< account_object >( [&]( account_object& a )
        {
            a.name = STEEMIT_TEMP_ACCOUNT;
        } );

        _db.create< account_authority_object >( [&]( account_authority_object& auth )
        {
            auth.account = STEEMIT_TEMP_ACCOUNT;
            auth.owner.weight_threshold = 0;
            auth.active.weight_threshold = 0;
        });

        // currently, the system will only create on initminer in below loop
        for( int i = 0; i < STEEMIT_NUM_INIT_MINERS; ++i )
        {

            _db.create< account_object >( [&]( account_object& a )
            {
            a.name = STEEMIT_INIT_MINER_NAME + ( i ? fc::to_string( i ) : std::string() );
            a.memo_key = init_public_key;
            a.balance  = asset( i ? 0 : init_supply, STEEM_SYMBOL );
            } );

            _db.create< account_authority_object >( [&]( account_authority_object& auth )
            {
            auth.account = STEEMIT_INIT_MINER_NAME + ( i ? fc::to_string( i ) : std::string() );
            auth.owner.add_authority( init_public_key, 1 );
            auth.owner.weight_threshold = 1;
            auth.active  = auth.owner;
            auth.posting = auth.active;
            });

            _db.create< witness_object >( [&]( witness_object& w )
            {
            w.owner        = STEEMIT_INIT_MINER_NAME + ( i ? fc::to_string(i) : std::string() );
            w.signing_key  = init_public_key;
            w.schedule = witness_object::miner;
            } );
        }

        // Nothing to do
        // QUESTION: Does this actually save and empty feed_history_object into index/container?
        _db.create< feed_history_object >( [&]( feed_history_object& o ) {});
        // QUESTION: Does it necessary to create all these empty objects?
        for( int i = 0; i < 0x10000; i++ )
            _db.create< block_summary_object >( [&]( block_summary_object& ) {});

        // during genesis, the total_vesting_shares and total_vesting_steem_fund are still 0
        _db.create< dynamic_global_property_object >( [&]( dynamic_global_property_object& p )
        {
            p.current_witness = STEEMIT_INIT_MINER_NAME;
            p.time = STEEMIT_GENESIS_TIME;
            p.recent_slots_filled = fc::uint128::max_value();
            p.participation_count = 128;
            p.current_supply = asset( init_supply, STEEM_SYMBOL );
            p.virtual_supply = p.current_supply;
            p.maximum_block_size = STEEMIT_MAX_BLOCK_SIZE;
        } );
        
        _db.create< hardfork_property_object >( [&](hardfork_property_object& hpo )
        {
            hpo.last_hardfork = 0;
        } );
        
        _db.create< witness_schedule_object >( [&]( witness_schedule_object& wso )
        {
            wso.current_shuffled_witnesses[0] = STEEMIT_INIT_MINER_NAME;
        } );
    }
    FC_CAPTURE_AND_RETHROW()
}

}}
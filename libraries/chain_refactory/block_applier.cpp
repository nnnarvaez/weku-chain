#include <wk/chain_refactory/block_applier.hpp>

using namespace steemit::chain;

namespace wk{namespace chain{

void database::update_last_irreversible_block()
{ 
    try {
        const dynamic_global_property_object& dpo = get_dynamic_global_properties();

    //Prior to voting taking over, we must be more conservative...
    if( _db.head_block_num() < STEEMIT_START_MINER_VOTING_BLOCK )
    {
        _db.modify( dpo, [&]( dynamic_global_property_object& _dpo )
        {
            if ( _db.head_block_num() > STEEMIT_MAX_WITNESSES )
                _dpo.last_irreversible_block_num = _db.head_block_num() - STEEMIT_MAX_WITNESSES;
        } );
    }
    else
    {
        const witness_schedule_object& wso = _db.get_witness_schedule_object();

        std::vector< const witness_object* > wit_objs;
        wit_objs.reserve( wso.num_scheduled_witnesses );
        //ilog("num_scheduled_witnesses: ${a}", ("a", wso.num_scheduled_witnesses));
        for( int i = 0; i < wso.num_scheduled_witnesses; i++ )
            wit_objs.push_back( &get_witness( wso.current_shuffled_witnesses[i] ) );

        static_assert( STEEMIT_IRREVERSIBLE_THRESHOLD > 0, "irreversible threshold must be nonzero" );

        // QUESTION: not sure what below 3 lines is about?
        // 1 1 1 2 2 2 2 2 2 2 -> 2     .7*10 = 7
        // 1 1 1 1 1 1 1 2 2 2 -> 1
        // 3 3 3 3 3 3 3 3 3 3 -> 3

        // if wit_objs.size() < 4, then offset will be always 0.
        size_t offset = ((STEEMIT_100_PERCENT - STEEMIT_IRREVERSIBLE_THRESHOLD) * wit_objs.size() / STEEMIT_100_PERCENT);
        //ilog("offset: ${a}", ("a", offset));
        std::nth_element( wit_objs.begin(), wit_objs.begin() + offset, wit_objs.end(),
            []( const witness_object* a, const witness_object* b )
            {
                return a->last_confirmed_block_num < b->last_confirmed_block_num;
            } );

        uint32_t new_last_irreversible_block_num = wit_objs[offset]->last_confirmed_block_num;

        // TODO: Need to remove the hardcode here. // IMPORTANT
        // only used to fix the two witnesses, and branch from each other and cannot reach concensus.
        // Alexey: hard code here for fix the less witness bug
        //if(offset < 1)
        //    new_last_irreversible_block_num = wit_objs.back()->last_confirmed_block_num;

        //ilog("witness: ${a}", ("a", std::string(wit_objs[offset]->owner)));
        //ilog("before: new_last_irreversible_block_num: ${a}", ("a", new_last_irreversible_block_num));
        //ilog("before: dpo.last_irreversible_block_num: ${a}", ("a", dpo.last_irreversible_block_num));

        if( new_last_irreversible_block_num > dpo.last_irreversible_block_num )
        {
            _db.modify( dpo, [&]( dynamic_global_property_object& _dpo )
            {
                _dpo.last_irreversible_block_num = new_last_irreversible_block_num;
            } );
        }
    }

    //ilog("after: dpo.last_irreversible_block_num: ${a}", ("a", dpo.last_irreversible_block_num));
    // Discards all undo history prior to revision
    _db.commit( dpo.last_irreversible_block_num );

    if( !( _db.get_node_properties().skip_flags & skip_block_log ) )
    {
        // output to block log based on new last irreverisible block num
        const auto& tmp_head = _db.block_log().head();
        uint64_t log_head_num = 0;

        if( tmp_head )
            log_head_num = tmp_head->block_num();
        ilog("log_head_num: ${a}", ("a", log_head_num));
        ilog("last irreversible block num: ${n}", ("n", dpo.last_irreversible_block_num));
        if( log_head_num < dpo.last_irreversible_block_num )
        {
            while( log_head_num < dpo.last_irreversible_block_num )
            {
                shared_ptr< fork_item > block = _db.fork_db().fetch_block_on_main_branch_by_number( log_head_num+1 );
                FC_ASSERT( block, "Current fork in the fork database does not contain the last_irreversible_block" );
                _db.block_log().append( block->data );
                ilog("append data to block log file.");
                log_head_num++;
            }

            _db.block_log().flush();
        }
    }

    // QUESTION: not sure what exactly below function is doing
    // doing this will resize the max_size of fork_db, which in turn delete old items out of the range of new _max_size.
    _db.fork_db().set_max_size( dpo.head_block_number - dpo.last_irreversible_block_num + 1 );
    } FC_CAPTURE_AND_RETHROW() 
}


void block_applier::update_signing_witness(const witness_object& signing_witness, const signed_block& new_block)
{ 
    try {
        const dynamic_global_property_object& dpo = _db.get_dynamic_global_properties();
        uint64_t new_block_aslot = dpo.current_aslot + _db.get_slot_at_time( new_block.timestamp );

        _db.modify( signing_witness, [&]( witness_object& _wit )
        {
            _wit.last_aslot = new_block_aslot;
            _wit.last_confirmed_block_num = new_block.block_num();
            //ilog("wit.owner: ${a}",("a",std::string( _wit.owner)));
            //ilog("new block num: ${a}", ("a", new_block.block_num()));
            //ilog("last_confirmed_block_num:${a}", ("a",_wit.last_confirmed_block_num));
        } );
    } FC_CAPTURE_AND_RETHROW() 
}

}}
#include <wk/chain_refactory/gpo_processor.hpp>

using namespace steemit::chain;

namespace wk{namespace chain{

void gpo_processor::update_global_dynamic_data( const signed_block& b )
{ 
    try {
        const dynamic_global_property_object& _dgp =
            _db.get_dynamic_global_properties();

        uint32_t missed_blocks = 0;
        if( _db.head_block_time() != fc::time_point_sec() )
        {
            missed_blocks = _db.get_slot_at_time( b.timestamp );
            assert( missed_blocks != 0 );
            missed_blocks--;
            for( uint32_t i = 0; i < missed_blocks; ++i )
            {
                const auto& witness_missed = _db.get_witness( _db.get_scheduled_witness( i + 1 ) );
                if(  witness_missed.owner != b.witness )
                {
                    _db.modify( witness_missed, [&]( witness_object& w )
                    {
                    w.total_missed++;
                    if( _db.has_hardfork( STEEMIT_HARDFORK_0_14 ) )
                    {
                        // automatically de-active witnesses if it missed over one day' block.
                        if( _db.head_block_num() - w.last_confirmed_block_num  > STEEMIT_BLOCKS_PER_DAY )
                        {
                            w.signing_key = public_key_type();
                            _db.push_virtual_operation( shutdown_witness_operation( w.owner ) );
                        }
                    }
                    } );
                }
            }
        }

        // dynamic global properties updating
        _db.modify( _dgp, [&]( dynamic_global_property_object& dgp )
        {
            // This is constant time assuming 100% participation. It is O(B) otherwise (B = Num blocks between update)
            for( uint32_t i = 0; i < missed_blocks + 1; i++ )
            {
                dgp.participation_count -= dgp.recent_slots_filled.hi & 0x8000000000000000ULL ? 1 : 0;
                dgp.recent_slots_filled = ( dgp.recent_slots_filled << 1 ) + ( i == 0 ? 1 : 0 );
                dgp.participation_count += ( i == 0 ? 1 : 0 );
            }

            dgp.head_block_number = b.block_num();
            dgp.head_block_id = b.id();
            dgp.time = b.timestamp;
            dgp.current_aslot += missed_blocks+1;
        } );

        if( !(_db.get_node_properties().skip_flags & skip_undo_history_check) )
        {
            STEEMIT_ASSERT( _dgp.head_block_number - _dgp.last_irreversible_block_num  < STEEMIT_MAX_UNDO_HISTORY, undo_database_exception,
                "The database does not have enough undo history to support a blockchain with so many missed blocks. "
                "Please add a checkpoint if you would like to continue applying blocks beyond this point.",
                ("last_irreversible_block_num",_dgp.last_irreversible_block_num)("head", _dgp.head_block_number)
                ("max_undo",STEEMIT_MAX_UNDO_HISTORY) );
        }

    } FC_CAPTURE_AND_RETHROW() 
}

}}
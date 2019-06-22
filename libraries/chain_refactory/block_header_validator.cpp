#include <weku/chain/block_header_validator.hpp>
#include <weku/chain/witness_objects.hpp>

namespace weku{namespace chain{

const witness_object& block_header_validator::validate_block_header( uint32_t skip, const signed_block& next_block )const
{ try {
   FC_ASSERT( _db.head_block_id() == next_block.previous, "", ("head_block_id",_db.head_block_id())("next.prev",next_block.previous) );
   FC_ASSERT( _db.head_block_time() < next_block.timestamp, "", ("head_block_time",_db.head_block_time())("next",next_block.timestamp)("blocknum",next_block.block_num()) );
   const witness_object& witness = _db.get_witness( next_block.witness );

   if( !(skip&skip_witness_signature) )
      FC_ASSERT( next_block.validate_signee( witness.signing_key ) );

   if( !(skip&skip_witness_schedule_check) )
   {
      uint32_t slot_num = get_slot_at_time(_db, next_block.timestamp );
      FC_ASSERT( slot_num > 0 );

      std::string scheduled_witness = get_scheduled_witness(_db, slot_num );

      FC_ASSERT( witness.owner == scheduled_witness, "Witness produced block at wrong time",
                 ("block witness",next_block.witness)("scheduled",scheduled_witness)("slot_num",slot_num) );
   }

   return witness;
} FC_CAPTURE_AND_RETHROW() }

}}
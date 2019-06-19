#include <weku/chain/slot.hpp>

namespace weku{namespace chain{

uint32_t slot::get_slot_at_time(fc::time_point_sec when)const
{
   fc::time_point_sec first_slot_time = get_slot_time( 1 );
   if( when < first_slot_time )
      return 0;
   return (when - first_slot_time).to_seconds() / STEEMIT_BLOCK_INTERVAL + 1;
}

fc::time_point_sec slot::get_slot_time(uint32_t slot_num)const
{
   if( slot_num == 0 )
      return fc::time_point_sec();

   auto interval = STEEMIT_BLOCK_INTERVAL;
   const dynamic_global_property_object& dpo = _db.get_dynamic_global_properties();

    // means just complete the init_genesis, which at block #0, the block #1 is not generated yet at this point.
   if( _db.head_block_num() == 0 )
   {
      // n.b. first block is at genesis_time plus one block interval
      fc::time_point_sec genesis_time = dpo.time;
      return genesis_time + slot_num * interval;
   }

   int64_t head_block_abs_slot = _db.head_block_time().sec_since_epoch() / interval;
   // re-adjust head_slot_time to absolute slot time based on unix epoch.
   fc::time_point_sec head_slot_time( head_block_abs_slot * interval );

   // "slot 0" is head_slot_time
   // "slot 1" is head_slot_time,
   //   plus maint interval if head block is a maint block
   //   plus block interval if head block is not a maint block
   return head_slot_time + (slot_num * interval);
}

}}
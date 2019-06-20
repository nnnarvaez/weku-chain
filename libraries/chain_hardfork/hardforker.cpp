#include<weku/chain/hardforker.hpp>
#include<weku/chain/hardfork_doer.hpp>
#include<weku/chain/hardfork_constants.hpp>

namespace weku{namespace chain {

bool hardforker::has_enough_hardfork_votes(
   const hardfork_votes_type& next_hardfork_votes, 
   uint32_t hardfork, uint32_t block_num) const
{
    const auto& search_target = std::make_pair(hardfork, block_num);
    uint32_t votes = 0;
    for(const auto& item : next_hardfork_votes)
        if(item.first == search_target.first && item.second == search_target.second)
            votes++;
            
    return votes >= 17;
}

void hardforker::process()
{
   hardfork_doer doer(_db);
    
   switch(_db.head_block_num())
   {        
      // hardfork 01 - 19 should be all processed after block #1, 
      // since we need to compatible with existing data based on previous STEEM code.      
      // at this code been implemented, hardfork21 already have been applied,
      // so we know exactly when hardfork 20 and hardfork 21 should be applied on which block.
      // that's why we can pinpoint it to happen on specific block num
      // but hardfork 22 is not happend yet, so we allow it to be triggered at a future block.
      case 1: 
         doer.do_hardforks_to_19();
         _db.last_hardfork(19);
         break;
      case HARDFORK_20_BLOCK_NUM:
         // NOTHING NEED TO BE DONE HERE
         _db.last_hardfork(20);
         break;
      case HARDFORK_21_BLOCK_NUM:
         doer.do_hardfork_21();
         _db.last_hardfork(21);
         break;        
   }

   // need to vote to trigger hardfork 22, and so to future hardforks
   if(_db.last_hardfork() == 21 && _db.head_block_num() >= HARDFORK_22_BLOCK_NUM_FROM) 
   {            
      if(has_enough_hardfork_votes(_db.next_hardfork_votes(), 22, HARDFORK_22_BLOCK_NUM_FROM)){
         doer.do_hardfork_22();
         _db.last_hardfork(22);
      }             
   }   

   _db.push_virtual_operation( hardfork_operation( _db.last_hardfork() ), true );
}

}}
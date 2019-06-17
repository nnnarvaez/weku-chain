#include <wk/chain_new/mock_hardforker.hpp>

namespace wk {namespace chain {

bool mock_hardforker::has_enough_hardfork_votes(const hardfork_votes_type& next_hardfork_votes, 
    uint32_t hardfork, uint32_t block_num) const
{
    const auto& search_target = std::make_pair(hardfork, block_num);
    uint32_t votes = 0;
    for(const auto& item : next_hardfork_votes)
        if(item.first == search_target.first && item.second == search_target.second)
            votes++;
            
    return votes >= REQUIRED_HARDFORK_VOTES;
}

void mock_hardforker::process(idatabase& db)
{
    switch(db.head_block_num())
    {        
        // hardfork 01 - 19 should be all processed after block #1, 
        // since we need to compatible with existing data based on previous STEEM code.
        
        // at this code been implemented, hardfork21 already have been applied,
        // so we know exactly when hardfork 20 and hardfork 21 should be applied on which block.
        // that's why we can pinpoint it to happen on specific block num
        // but hardfork 22 is not happend yet, so we allow it to be triggered at a future block.
        case 1: 
            //apply_hardforks_to_19(db);
            db.last_hardfork(HARDFORK_19);
            break;
        case HARDFORK_20_BLOCK_NUM:
            // do_hardfork_20();
            db.last_hardfork(HARDFORK_20);
            break;
        case HARDFORK_21_BLOCK_NUM:
            // DO_AHRDFORK_21();
            db.last_hardfork(HARDFORK_21);
            break;        
    }

    if(db.last_hardfork() == HARDFORK_21 && db.head_block_num() >= HARDFORK_22_BLOCK_NUM) // need to vote to trigger hardfork 22, and so to future hardforks
    {            
        if(has_enough_hardfork_votes(db.next_hardfork_votes(), HARDFORK_22, HARDFORK_22_BLOCK_NUM)){
            //apply_hardfork_22(db);
            db.last_hardfork(HARDFORK_22);
        }             
    }   
}

}}
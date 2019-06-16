
#include <wk/chain_new/x_database.hpp>

namespace wk{namespace chain_new{

bool has_enough_hardfork_votes(const hardfork_votes_type& next_hardfork_votes, uint32_t hardfork, uint32_t block_num)
{
    const auto& search_target = std::make_pair(hardfork, block_num);
    uint32_t votes = 0;
    for(const auto& item : next_hardfork_votes)
        if(item.first == search_target.first && item.second == search_target.second)
            votes++;
            
    return votes >= REQUIRED_HARDFORK_VOTES;
}

void process_hardfork(x_database& _db)
{
    switch(_db.head_block_num())
    {        
        // hardfork 01 - 19 should be all processed after block #1, 
        // since we need to compatible with existing data based on previous STEEM code.
        
        // at this code been implemented, hardfork21 already have been applied,
        // so we know exactly when hardfork 20 and hardfork 21 should be applied on which block.
        // that's why we can pinpoint it to happen on specific block num
        // but hardfork 22 is not happend yet, so we allow it to be triggered at a future block.
        case 1u: 
            //apply_hardforks_to_19(_db);
            _db.last_hardfork(HARDFORK_19);
            break;
        case HARDFORK_20_BLOCK_NUM:
            // do_hardfork_20();
            _db.last_hardfork(HARDFORK_20);
            break;
        case HARDFORK_21_BLOCK_NUM:
            // DO_AHRDFORK_21();
            _db.last_hardfork(HARDFORK_21);
            break;        
    }

    if(_db.last_hardfork() == HARDFORK_21 && _db.head_block_num() >= HARDFORK_22_BLOCK_NUM) // need to vote to trigger hardfork 22, and so to future hardforks
    {            
        if(has_enough_hardfork_votes(_db.next_hardfork_votes(), HARDFORK_22, HARDFORK_22_BLOCK_NUM)){
            //apply_hardfork_22(_db);
            _db.last_hardfork(HARDFORK_22);
        }             
    }   
}

void x_database::init_genesis(uint64_t initial_supply)
{
}

uint32_t x_database::head_block_num() const
{
    return _head_block_num;
}

std::vector<signed_block>& x_database::ledger_blocks()
{
    return _ledger_blocks;
}

const hardfork_votes_type& x_database::next_hardfork_votes() const
{
    return _next_hardfork_votes;
}

// void x_database::apply_hardfork(uint32_t hardfork)
// {
//     if (hardfork == 0)
//         throw std::runtime_error("cannot apply hardfork 0");
//     _last_hardfork = hardfork;
// }

bool x_database::apply_block(const signed_block& b, uint32_t skip){

    _ledger_blocks.push_back(b);

    _head_block_num++;
    
    process_hardfork(*this);
        
    return true;
}

void x_database::replay_chain()
{

}

signed_block x_database::generate_block_before_apply()
{
    signed_block b;
    return b;
}

signed_block x_database::generate_block()
{
    signed_block b = generate_block_before_apply();
    apply_block(b);    
    return b;
}

signed_block x_database::debug_generate_block_until(uint32_t block_num)
{
    signed_block b;
    for(uint32_t i = 1; i < block_num; i++)
        generate_block();
    return b;
}

void x_database::debug_set_next_hardfork_votes(hardfork_votes_type next_hardfork_votes)
{
    _next_hardfork_votes = next_hardfork_votes;
}
    
}}
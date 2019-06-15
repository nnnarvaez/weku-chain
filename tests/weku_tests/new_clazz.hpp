#pragma once

#include <cstdlib>
#include <vector>

#define HARDFORK_19 19u
#define HARDFORK_19_BLOCK_NUM 1u // apply hardfork 19 on block 1
#define HARDFORK_20 20u
#define HARDFORK_20_BLOCK_NUM 200u
#define HARDFORK_21 21u
#define HARDFORK_21_BLOCK_NUM 300u
#define HARDFORK_22 22u
#define HARDFORK_22_BLOCK_NUM 400u
#define REQUIRED_HARDFORK_VOTES 17u

typedef std::vector<std::pair<uint32_t, uint32_t>> hardfork_votes_type;

struct signed_block
{
};

class x_database
{
public:
    void init_genesis();
    uint32_t head_block_num(); 
    std::vector<signed_block>& ledger_blocks();

    uint32_t last_hardfork();  
    const hardfork_votes_type& next_hardfork_votes() const;
    void process_hardforks_until_19(); 
    bool has_enough_hardfork_votes(uint32_t hardfork, uint32_t block_num);
    void apply_hardfork(uint32_t hardfork);
    
    bool apply_block(const signed_block&);
    signed_block generate_block_before_apply();
    signed_block generate_block();
    
    void replay_chain();
    
    signed_block debug_generate_block_until(uint32_t block_num);
    void debug_set_next_hardfork_votes(hardfork_votes_type next_hardfork_votes);
private:
    uint32_t _head_block_num = 0;
    uint32_t _last_hardfork = 0;
    std::vector<signed_block> _ledger_blocks;
    hardfork_votes_type _next_hardfork_votes; // TODO: move to a dependent object, and extract data from shuffled witnesses
};

void x_database::init_genesis()
{
}

uint32_t x_database::head_block_num(){
    return _head_block_num;
}

std::vector<signed_block>& x_database::ledger_blocks()
{
    return _ledger_blocks;
}

uint32_t x_database::last_hardfork()
{
    return _last_hardfork;
}

const hardfork_votes_type& x_database::next_hardfork_votes() const
{
    return _next_hardfork_votes;
}

void x_database::process_hardforks_until_19(){
    _last_hardfork = 19;
}

bool x_database::has_enough_hardfork_votes(uint32_t hardfork, uint32_t block_num)
{
    auto search_target = std::make_pair(hardfork, block_num);
    uint32_t votes = 0;
    for(const auto& item : next_hardfork_votes())
        if(item.first == search_target.first && item.second == search_target.second)
            votes++;
            
    return votes >= REQUIRED_HARDFORK_VOTES;
}

void x_database::apply_hardfork(uint32_t hardfork)
{
    if (hardfork == 0)
        throw std::runtime_error("cannot apply hardfork 0");
    _last_hardfork = hardfork;
}

bool x_database::apply_block(const signed_block& b){

    _ledger_blocks.push_back(b);

    _head_block_num++;
    
    switch(_head_block_num)
    {        
        // hardfork 01 - 19 should be all processed after block #1, 
        // since we need to compatible with existing data based on previous STEEM code.
        
        // at this code been implemented, hardfork21 already have been applied,
        // so we know exactly when hardfork 20 and hardfork 21 should be applied on which block.
        // that's why we can pinpoint it to happen on specific block num
        // but hardfork 22 is not happend yet, so we allow it to be triggered at a future block.
        case 1u: 
            process_hardforks_until_19();
            break;
        case HARDFORK_20_BLOCK_NUM:
            // do_hardfork_20();
            _last_hardfork = HARDFORK_20;
            break;
        case HARDFORK_21_BLOCK_NUM:
            // DO_AHRDFORK_21();
            _last_hardfork = HARDFORK_21;
            break;        
    }

    if(last_hardfork() == HARDFORK_21 && head_block_num() >= HARDFORK_22_BLOCK_NUM) // need to vote to trigger hardfork 22, and so to future hardforks
    {            
        if(has_enough_hardfork_votes(HARDFORK_22, HARDFORK_22_BLOCK_NUM)){
            // DO_AHRDFORK_22();
            _last_hardfork = HARDFORK_22;
        }
             
    }      
        
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

struct x_database_fixture{
    x_database db;

    x_database_fixture()
    {
        //std::cout << "test suite setup.";
        db.init_genesis();
    };
    ~x_database_fixture()
    {
        //std::cout << "test suite teardown.";
    };
};
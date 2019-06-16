
#include <wk/chain_new/x_database.hpp>
#include <wk/chain_new/hardforker.hpp>

namespace wk{namespace chain_new{

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
    
    hardforker hf;
    hf.process(*this);
        
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
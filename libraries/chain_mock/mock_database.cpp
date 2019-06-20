
#include <wk/chain_new/mock_database.hpp>

namespace weku{namespace chain{

void mock_database::init_genesis(uint64_t initial_supply)
{
}

uint32_t mock_database::head_block_num() const
{
    return _head_block_num;
}

const hardfork_votes_type& mock_database::next_hardfork_votes() const
{
    return _next_hardfork_votes;
}

bool mock_database::apply_block(const signed_block& b, uint32_t skip){

    _ledger_blocks.push_back(b);

    _head_block_num++;

    mock_hardforker hf;
    hf.process(*this);
        
    return true;
}

void mock_database::replay_chain()
{

}

signed_block mock_database::generate_block_before_apply()
{
    signed_block b;
    return b;
}

signed_block mock_database::generate_block()
{
    signed_block b = generate_block_before_apply();
    apply_block(b);    
    return b;
}

signed_block mock_database::debug_generate_block_until(uint32_t block_num)
{
    signed_block b;
    for(uint32_t i = 1; i < block_num; i++)
        generate_block();
    return b;
}

void mock_database::debug_set_next_hardfork_votes(hardfork_votes_type next_hardfork_votes)
{
    _next_hardfork_votes = next_hardfork_votes;
}
    
}}
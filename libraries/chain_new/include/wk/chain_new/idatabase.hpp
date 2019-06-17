#pragma once
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

typedef std::vector<std::pair<uint32_t, uint32_t> > hardfork_votes_type;
struct signed_block{};

namespace wk { namespace chain {
class idatabase // interface database
{
public:
    virtual ~idatabase() = default;

    virtual void init_genesis(uint64_t initial_supply = 0) = 0;
    virtual uint32_t head_block_num() const = 0; 
    virtual std::vector<signed_block>& ledger_blocks() = 0; // TODO: removed later.

    virtual uint32_t last_hardfork() const = 0;  
    virtual void last_hardfork(uint32_t hardfork) = 0; // TODO: need to encapsulate it.
    virtual const hardfork_votes_type& next_hardfork_votes() const = 0;  
    
    virtual signed_block generate_block_before_apply() = 0;
    virtual signed_block generate_block() = 0; // need refactory this interface.
    virtual bool apply_block(const signed_block& b, uint32_t skip = 0) = 0;
    
    virtual void replay_chain() = 0;
    
    virtual signed_block debug_generate_block_until(uint32_t block_num) = 0;
    virtual void debug_set_next_hardfork_votes(hardfork_votes_type next_hardfork_votes) = 0;
};

}}


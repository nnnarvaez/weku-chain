#pragma once
#include "idatabase.hpp"

namespace wk{namespace chain{

class mock_database: public idatabase
{
public:
    virtual void init_genesis(uint64_t initial_supply = 0) override;
    virtual uint32_t head_block_num() const override; 
    virtual std::vector<signed_block>& ledger_blocks() override; // TODO: removed later.

    virtual uint32_t last_hardfork() const  override{return _last_hardfork;};  
    virtual void last_hardfork(uint32_t hardfork)  override{_last_hardfork = hardfork;} // TODO: need to encapsulate it.
    virtual const hardfork_votes_type& next_hardfork_votes() const override;  
    
    virtual signed_block generate_block_before_apply() override;
    virtual signed_block generate_block() override; // need refactory this interface.
    virtual bool apply_block(const signed_block& b, uint32_t skip = 0) override;
    
    virtual void replay_chain() override;
    
    virtual signed_block debug_generate_block_until(uint32_t block_num) override;
    virtual void debug_set_next_hardfork_votes(hardfork_votes_type next_hardfork_votes) override;
private:
    uint32_t _head_block_num = 0;
    uint32_t _last_hardfork = 0;
    std::vector<signed_block> _ledger_blocks;
    hardfork_votes_type _next_hardfork_votes; // TODO: move to a dependent object, and extract data from shuffled witnesses
};

}}
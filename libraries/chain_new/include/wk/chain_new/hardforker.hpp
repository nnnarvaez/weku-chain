#pragma once

#include <wk/chain_new/ihardforker.hpp>

namespace wk {namespace chain_new {

class hardforker: public ihardforker{
    public:    
    virtual bool has_enough_hardfork_votes(const hardfork_votes_type& next_hardfork_votes, 
        uint32_t hardfork, uint32_t block_num) const override;
    virtual void process(idatabase& db) override;
};

}}

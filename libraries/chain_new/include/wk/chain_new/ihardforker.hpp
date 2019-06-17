#pragma once

#include <wk/chain_new/idatabase.hpp>

namespace wk {namespace chain {
    class ihardforker{
        public:
        virtual ~ihardforker() = default;
        virtual bool has_enough_hardfork_votes(const hardfork_votes_type& next_hardfork_votes, 
            uint32_t hardfork, uint32_t block_num) const = 0;
        virtual void process(idatabase& db) = 0;
    };
}}
#pragma once
#include <vector>
#include <utility>
#include <weku/chain/shared_types.hpp>

namespace weku{namespace chain{
    class i_hardforker{
        public:
        virtual ~i_hardforker() = default;
        
        virtual bool has_enough_hardfork_votes(
            const hardfork_votes_type& next_hardfork_votes, 
            uint32_t hardfork, uint32_t block_num) const = 0;
        
        virtual void process() = 0;
    };
}}
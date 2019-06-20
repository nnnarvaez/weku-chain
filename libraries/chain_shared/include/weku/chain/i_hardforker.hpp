#pragma once
#include <vector>
#include <utility>

namespace weku{namespace chain{
    typedef std::vector<std::pair<uint32_t, uint32_t> > hardfork_votes_type;

    class i_hardforker{
        public:
        virtual ~i_hardforker() = default;
        
        virtual bool has_enough_hardfork_votes(
            const hardfork_votes_type& next_hardfork_votes, 
            uint32_t hardfork, uint32_t block_num) const = 0;
        
        virtual void process() = 0;
    };
}}
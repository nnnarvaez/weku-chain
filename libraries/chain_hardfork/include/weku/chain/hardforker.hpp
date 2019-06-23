#pragma once

#include <weku/chain/i_hardfork_doer.hpp>
#include <weku/chain/i_hardfork_voter.hpp>

namespace weku{namespace chain{
    class hardforker: public i_hardforker{
        public:
        hardforker(i_hardfork_voter& voter, i_hardfork_doer& doer):_voter(voter), _doer(doer){}
        
        virtual bool has_enough_hardfork_votes(
            const hardfork_votes_type& next_hardfork_votes, 
            uint32_t hardfork, uint32_t block_num) const override;
        virtual uint32_t process(const uint32_t head_block_num, const uint32_t last_hardfork) override;

        private:        
            i_hardfork_voter& _voter;    
            i_hardfork_doer& _doer;            
    };
}}
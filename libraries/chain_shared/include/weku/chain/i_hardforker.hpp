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

        virtual void perform_vesting_share_split( uint32_t magnitude ) = 0;
        virtual void retally_witness_vote_counts( bool force = false ) = 0;
        virtual void retally_witness_votes() = 0;
        virtual void reset_virtual_schedule_time() = 0;
        virtual void do_hardfork_12() = 0;
        virtual void do_hardfork_17() = 0;
        virtual void do_hardfork_19() = 0;
        virtual void do_hardfork_21() = 0;
        virtual void do_hardfork_22() = 0;
        virtual void do_hardforks_to_19() = 0; 
    };
}}
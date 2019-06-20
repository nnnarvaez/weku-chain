#pragma once

#include <weku/chain/i_hardforker.hpp>
#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{
    class hardforker: i_hardforker{
        public:
        hardforker(itemp_database& db):_db(db){}
        
        virtual bool has_enough_hardfork_votes(
            const hardfork_votes_type& next_hardfork_votes, 
            uint32_t hardfork, uint32_t block_num) const;
        virtual void process() override;

        virtual void perform_vesting_share_split( uint32_t magnitude ) override;
        virtual void retally_witness_vote_counts( bool force = false ) override;
        virtual void retally_witness_votes() override;
        virtual void reset_virtual_schedule_time() override;
        virtual void do_hardfork_12() override;
        virtual void do_hardfork_17() override;
        virtual void do_hardfork_19() override;
        virtual void do_hardfork_21() override;
        virtual void do_hardfork_22() override;
        virtual void do_hardforks_to_19() override;        

        private:
            itemp_database& _db;
    };
}}
#pragma once

#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{
    class hardforker{
        public:
        hardforker(itemp_database& db):_db(db){}
        
        bool has_enough_hardfork_votes(
            const hardfork_votes_type& next_hardfork_votes, 
            uint32_t hardfork, uint32_t block_num) const;
        void process();

        void perform_vesting_share_split( uint32_t magnitude );
        void retally_witness_vote_counts( bool force = false );
        void retally_witness_votes();
        void reset_virtual_schedule_time();
        void do_hardfork_12();
        void do_hardfork_17();
        void do_hardfork_19();
        void do_hardfork_21();
        void do_hardfork_22();
        void do_hardforks_to_19();        

        private:
            itemp_database& _db;
    };
}}
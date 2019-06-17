#pragma once

#include <steemit/chain/itemp_database.hpp>

namespace steemit{namespace chain{
    class hardfork_doer{
        public:
        hardfork_doer(itemp_database& db):_db(db){}

        virtual bool has_enough_hardfork_votes(const hardfork_votes_type& next_hardfork_votes, 
        uint32_t hardfork, uint32_t block_num) const;
        void process();
        void do_hardforks_to_19();

        // for hardfork 6 and 8
        void retally_witness_vote_counts( bool force = false );
        void retally_witness_votes();
        void retally_comment_children();
        void do_hardfork_01_perform_vesting_share_split( uint32_t magnitude );
        void retally_liquidity_weight();
        void do_hardfork_12();
        void do_hardfork_17();
        void do_hardfork_19();
        void do_hardfork_21();
        void do_hardfork_22();

        private:
            itemp_database& _db;
    };
}}
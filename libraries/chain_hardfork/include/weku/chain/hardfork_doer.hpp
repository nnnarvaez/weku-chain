#pragma once
#include <weku/chain/itemp_database.hpp>
#include <weku/chain/i_hardfork_doer.hpp>

namespace weku{namespace chain{
    class hardfork_doer: public i_hardfork_doer{
        public:
        hardfork_doer(itemp_database& db):_db(db){}
        virtual void do_hardforks_to_19() override;        
        virtual void do_hardfork_21() override;
        virtual void do_hardfork_22() override;

        private:
        void perform_vesting_share_split( uint32_t magnitude );
        void retally_witness_vote_counts( bool force = false );
        void retally_witness_votes() ;
        void reset_virtual_schedule_time() ;
        void do_hardfork_12();
        void do_hardfork_17();
        void do_hardfork_19();

        private:
            itemp_database& _db;
    };
}}
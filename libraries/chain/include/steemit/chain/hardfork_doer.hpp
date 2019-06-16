#pragma once

#include <steemit/chain/itemp_database.hpp>

namespace steemit{namespace chain{
    class hardfork_doer{
        public:
        hardfork_doer(itemp_database& db):_db(db){}

        // for hardfork 6 and 8
        void retally_witness_vote_counts( bool force = false );

        private:
            itemp_database& _db;

    };
}}
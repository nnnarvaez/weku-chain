#pragma once

#include <steemit/chain/itemp_database.hpp>

namespace steemit{namespace chain{
    class invariant_avlidator{
        public:
            invariant_validator(itemp_database& db):_db(db){}
            void validate();
        private:
            itemp_database& _db;
    };
}}
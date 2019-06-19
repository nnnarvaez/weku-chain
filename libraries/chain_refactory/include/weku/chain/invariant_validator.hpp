#pragma once
#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{
    class invariant_validator{
        public:
            invariant_validator(const itemp_database& db):_db(db){}
            void validate();
        private:
            const itemp_database& _db;
    };
}}
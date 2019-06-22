#pragma once

#include <weku/chain/database.hpp>

namespace weku{namespace chain{
    class indexes_initializer{
        public:
            indexes_initializer(database& db):_db(db){}
            void initialize_indexes();
        private:
            database& _db;
    };
}}
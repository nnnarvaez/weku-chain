#pragma once

namespace weku{namespace chain{
    class indexes_initializer{
        public:
            indexes_initializer(itemp_database& db):_db(db){}
            void initialize_indexes();
        private:
            itemp_database& _db;
    };
}}
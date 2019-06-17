#pragma once

#include <wk/chain_refactory/itemp_database.hpp>
using namespace wk::chain;
namespace steemit{namespace chain{
    class invariant_avlidator{
        public:
            invariant_validator(itemp_database& db):_db(db){}
            void validate();
        private:
            itemp_database& _db;
    };
}}
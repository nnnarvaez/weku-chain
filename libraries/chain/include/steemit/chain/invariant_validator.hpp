#pragma once

#include <wk/chain_refactory/itemp_database.hpp>
using namespace wk::chain;
namespace steemit{namespace chain{
    class invariant_validator{
        public:
            invariant_validator(const itemp_database& db):_db(db){}
            void validate();
        private:
            const itemp_database& _db;
    };
}}
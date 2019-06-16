#pragma once
#include <steemit/chainbase/chainbase.hpp>

namespace steemit{ namespace chain{
    // this class is to tempararily used to refactory database
    class itemp_database: public chainbase::database 
    {
        virtual ~itemp_database() = default;
        virtual void foo() = 0;
    };
}}
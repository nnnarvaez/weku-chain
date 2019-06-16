#pragma once

namespace steemit{ namespace chain{
    class temp_database: public itemp_database
    {
        public:
        virtual void foo() override{}
    };
}}
#pragma once

namespace weku{namespace chain{
    class i_hardfork_doer{
        public:
        virtual ~i_hardfork_doer() = default;
        
        virtual void do_hardforks_to_19() = 0;        
        virtual void do_hardfork_21() = 0;
        virtual void do_hardfork_22() = 0;
    };
}}
#pragma once

namespace wk {namespace chain_new {
    class ihardforker{
        public:
        virtual ~ihardforker() = default;

        virtual void process() = 0;
    };
}}
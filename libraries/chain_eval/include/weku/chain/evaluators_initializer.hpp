#pragma once

#include <weku/chain/weku_evaluator.hpp>
#include <weku/chain/evaluator_registry.hpp>

namespace weku{namespace chain{
    class evaluators_initializer{
        public:
        evaluators_initializer(evaluator_registry< operation >& registry):_registry(registry){}
        void initialize_evaluators();
        private:
            evaluator_registry< operation >& _registry;
    };
}}
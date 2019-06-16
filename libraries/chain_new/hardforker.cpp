#include <wk/chain_new/ihardforker.hpp>

namespace wk {namespace chain_new {
    class hardforker: public ihardforker{
        public:    

        virtual void process() override;
    };

    void hardforker::process(){
        
    }
}}
#pragma once

#include <weku/chain/i_hardforker.hpp>
#include <weku/chain/itemp_database.hpp>
#include <weku/chain/i_hardfork_doer.hpp>

namespace weku{namespace chain{
    class hardforker: public i_hardforker{
        public:
        hardforker(itemp_database& db, i_hardfork_doer& doer):_db(db), _doer(doer){}
        
        virtual bool has_enough_hardfork_votes(
            const hardfork_votes_type& next_hardfork_votes, 
            uint32_t hardfork, uint32_t block_num) const override;
        virtual void process() override;    

        private:
            itemp_database& _db;
            i_hardfork_doer& _doer;
    };
}}
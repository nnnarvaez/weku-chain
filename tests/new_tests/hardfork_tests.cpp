#include "gmock/gmock.h"

#include <weku/chain/i_hardfork_doer.hpp>
#include <weku/chain/hardforker.hpp>

namespace weku{namespace chain{

class mock_database: public itemp_database
{
    public:
    MOCK_METHOD0(last_hardfork, uint32_t());
      
};

class mock_hardfork_doer: public i_hardfork_doer{
public:

};

}}

using namespace weku::chain;

TEST(hardfork_19, should_be_triggerred_at_block_1){
    mock_database db;
    mock_hardfork_doer doer;    
    hardforker hfkr(db, dore);
}

// TEST(hardfork_22, should_not_trigger_without_enough_votes){
//     mock_database db;
//     hardforker hfr(db);
// }
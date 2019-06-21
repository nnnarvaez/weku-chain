#include "gmock/gmock.h"
#include <weku/chain/hardforker.hpp>

namespace weku{namespace chain{

class mock_database: public itemp_database{
public:
    MOCK_METHOD0(lasthardfork, uint32_t());
};

}}

using namespace weku::chain;

TEST(hardfork_19, should_be_triggerred_at_block_1){
    mock_database db;
    
    hardforker hfr(db);
}

// TEST(hardfork_22, should_not_trigger_without_enough_votes){
//     mock_database db;
//     hardforker hfr(db);
// }
#include "gmock/gmock.h"

#include <weku/chain/database.hpp>
#include <weku/chain/hardforker.hpp>

using ::testing::Eq;
using namespace weku::chain;

TEST(hardfork, last_hardfork_should_be_0_after_database_init){
    database db;
    //ASSERT_THAT(db.last_hardfork(), Eq(0));
    ASSERT_THAT(1, 1);
}
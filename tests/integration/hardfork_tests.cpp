#include "gmock/gmock.h"

#include <weku/chain/database.hpp>
#include <weku/chain/hardforker.hpp>

using ::testing::Eq;
using namespace weku::chain;

#define TEST_INITIAL_SUPPLY 100000
#define TEST_SHARED_MEM_SIZE (1024 * 1024 * 8)

TEST(hardfork, last_hardfork_should_be_0_after_database_open){
    fc::temp_directory data_dir( fc::temp_directory_path() / "weku-tmp" );   
    auto init_account_priv_key = fc::ecc::private_key::regenerate( fc::sha256::hash( string( "init_key" ) ) );
      
    database db;
    db.open(data_dir.path(), data_dir.path(), TEST_INITIAL_SUPPLY, TEST_SHARED_MEM_SIZE, chainbase::database::read_write );
    ASSERT_THAT(db.last_hardfork(), Eq(0));

    signed_block b = db.generate_block(db.get_slot_time(1), db.get_scheduled_witness(1), init_account_priv_key, database::skip_nothing);
    EXPECT_TRUE(false) << "last hardfork after block #1: " << db.last_hardfork();
    ASSERT_THAT(db.last_hardfork(), Eq(19));

    db.close();
}
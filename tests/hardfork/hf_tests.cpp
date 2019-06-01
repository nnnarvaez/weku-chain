#ifdef IS_TEST_NET
#include <boost/test/unit_test.hpp>

#include <steemit/protocol/exceptions.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/history_object.hpp>

#include <steemit/account_history/account_history_plugin.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace steemit;
using namespace steemit::chain;
using namespace steemit::protocol;

#define TEST_SHARED_MEM_SIZE (1024 * 1024 * 8)

BOOST_AUTO_TEST_SUITE(block_tests)

BOOST_FIXTURE_TEST_CASE( hardfork_test, database_fixture )
{
   try
   {
      try {
      int argc = boost::unit_test::framework::master_test_suite().argc;
      char** argv = boost::unit_test::framework::master_test_suite().argv;
      for( int i=1; i<argc; i++ )
      {
         const std::string arg = argv[i];
         if( arg == "--record-assert-trip" )
            fc::enable_record_assert_trip = true;
         if( arg == "--show-test-names" )
            std::cout << "running test " << boost::unit_test::framework::current_test_case().p_name << std::endl;
      }
      auto ahplugin = app.register_plugin< steemit::account_history::account_history_plugin >();
      db_plugin = app.register_plugin< steemit::plugin::debug_node::debug_node_plugin >();
      init_account_pub_key = init_account_priv_key.get_public_key();

      boost::program_options::variables_map options;

      ahplugin->plugin_initialize( options );
      db_plugin->plugin_initialize( options );

      open_database(); // steps: init_genesis (set head_block_time() to genesis_time, push genesis time to hpo.processed_hardforks) -> init_hardforks

      generate_blocks( 2 );

      ahplugin->plugin_startup();
      db_plugin->plugin_startup();

      vest( "initminer", 10000 );

      // Fill up the rest of the required miners
      for( int i = STEEMIT_NUM_INIT_MINERS; i < STEEMIT_MAX_WITNESSES; i++ )
      {
         account_create( STEEMIT_INIT_MINER_NAME + fc::to_string( i ), init_account_pub_key );
         fund( STEEMIT_INIT_MINER_NAME + fc::to_string( i ), STEEMIT_MIN_PRODUCER_REWARD.amount.value );
         witness_create( STEEMIT_INIT_MINER_NAME + fc::to_string( i ), init_account_priv_key, "foo.bar", init_account_pub_key, STEEMIT_MIN_PRODUCER_REWARD.amount );
      }

      validate_database();
      } catch ( const fc::exception& e )
      {
         edump( (e.to_detail_string()) );
         throw;
      }

      BOOST_TEST_MESSAGE( "Check hardfork not applied at genesis" );
      BOOST_REQUIRE( db.has_hardfork( 0 ) );
      BOOST_REQUIRE( !db.has_hardfork( STEEMIT_HARDFORK_0_1 ) );

      BOOST_TEST_MESSAGE( "Generate blocks up to the hardfork time and check hardfork still not applied" );
      generate_blocks( fc::time_point_sec( STEEMIT_HARDFORK_0_1_TIME - STEEMIT_BLOCK_INTERVAL ), true );

      BOOST_REQUIRE( db.has_hardfork( 0 ) );
      BOOST_REQUIRE( !db.has_hardfork( STEEMIT_HARDFORK_0_1 ) );

      BOOST_TEST_MESSAGE( "Generate a block and check hardfork is applied" );
      generate_block();

      string op_msg = "Testnet: Hardfork applied";
      auto itr = db.get_index< account_history_index >().indices().get< by_id >().end();
      itr--;

      BOOST_REQUIRE( db.has_hardfork( 0 ) );
      BOOST_REQUIRE( db.has_hardfork( STEEMIT_HARDFORK_0_1 ) );
      BOOST_REQUIRE( get_last_operations( 1 )[0].get< custom_operation >().data == vector< char >( op_msg.begin(), op_msg.end() ) );
      BOOST_REQUIRE( db.get(itr->op).timestamp == db.head_block_time() );

      BOOST_TEST_MESSAGE( "Testing hardfork is only applied once" );
      generate_block();

      itr = db.get_index< account_history_index >().indices().get< by_id >().end();
      itr--;

      BOOST_REQUIRE( db.has_hardfork( 0 ) );
      BOOST_REQUIRE( db.has_hardfork( STEEMIT_HARDFORK_0_1 ) );
      BOOST_REQUIRE( get_last_operations( 1 )[0].get< custom_operation >().data == vector< char >( op_msg.begin(), op_msg.end() ) );
      BOOST_REQUIRE( db.get(itr->op).timestamp == db.head_block_time() - STEEMIT_BLOCK_INTERVAL );
   }
   FC_LOG_AND_RETHROW()
}

BOOST_AUTO_TEST_SUITE_END()
#endif

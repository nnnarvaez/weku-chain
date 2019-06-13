#include <boost/test/unit_test.hpp>

#include <steemit/protocol/exceptions.hpp>

#include <steemit/chain/database.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <steemit/chain/history_object.hpp>

#include <steemit/account_history/account_history_plugin.hpp>

#include <graphene/utilities/tempdir.hpp>

#include <fc/crypto/digest.hpp>

#include "../common/database_fixture.hpp"

using namespace fc;
using namespace steemit;
using namespace steemit::chain;
using namespace steemit::protocol;

struct new_hardfork_property_object{    // should be merge into gpo
   static std::vector<fc::time_point_sec> hardfork_times; // TODO: add const later

   uint32_t current_hardfork = 0; // updated by apply_hardfork
   fc::time_point_sec head_block_time = STEEMIT_GENESIS_TIME; 
   uint32_t next_hardfork    = 0; // updated by witness_schedule
   fc::time_point_sec next_hardfork_time = time_point_sec::maximum(); // updated by witness_schedule
};

std::vector<fc::time_point_sec> new_hardfork_property_object::hardfork_times{
   fc::time_point_sec( STEEMIT_GENESIS_TIME ), // hardfork 0 for genesis
   fc::time_point_sec( STEEMIT_HARDFORK_0_1_TIME ), // hardfork 1, starts at block #1
   fc::time_point_sec( STEEMIT_HARDFORK_0_2_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_3_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_4_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_5_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_6_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_7_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_8_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_9_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_10_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_11_TIME ),   
   fc::time_point_sec( STEEMIT_HARDFORK_0_12_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_13_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_14_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_15_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_16_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_17_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_18_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_19_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_20_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_21_TIME ),
   fc::time_point_sec( STEEMIT_HARDFORK_0_22_TIME )
};

bool has_hardfork(const new_hardfork_property_object& hpo, uint32_t hardfork){
   return hpo.current_hardfork >= hardfork;
}

// before hf22 (not include hf22), system uses current_hardfork and hardfork_times to trigger hardfork
// after hf22( include hf22), system uses current_hardfork and next_hardfork to trigger hardfork.
/* 
bool should_hardfork_triggered(const new_hardfork_property_object& hpo){
   if(has_hardfork(hpo, STEEMIT_HARDFORK_0_21))      
      return hpo.current_hardfork < hpo.next_hardfork 
         && hpo.head_block_time >= hpo.next_hardfork_time;
   else
      return hpo.current_hardfork < STEEMIT_HARDFORK_0_21 
         // _hardfork_times[ hpo.current_hardfork + 1 ] means next_hardfork_time
         && hpo.head_block_time >= new_hardfork_property_object::hardfork_times[ hpo.current_hardfork + 1 ]; 
}*/

void apply_hardfork(new_hardfork_property_object& hpo, uint32_t hardfork){
   // other processes...
   BOOST_TEST_MESSAGE("applied hardfork " << hardfork);
   hpo.current_hardfork = hardfork;
   //hpo.head_block_time = head_block_time();
}

void process_hardforks(new_hardfork_property_object& hpo){
   // before finishing init_genesis, the process_hardforks should be never invoked.
   if(hpo.head_block_time == time_point_sec(STEEMIT_GENESIS_TIME)) return;
   
      if(hpo.current_hardfork < STEEMIT_HARDFORK_0_21){
         for(uint32_t i = 1; i <= STEEMIT_HARDFORK_0_21; i++)
            if(hpo.current_hardfork < STEEMIT_HARDFORK_0_21
               && hpo.head_block_time >= new_hardfork_property_object::hardfork_times[hpo.current_hardfork + 1])
               apply_hardfork(hpo, hpo.current_hardfork + 1);   
      }else{ // after hardfork 21 applied
         for(uint32_t i = STEEMIT_HARDFORK_0_22; i <= STEEMIT_NUM_HARDFORKS; i++)
            if(hpo.current_hardfork < hpo.next_hardfork && hpo.head_block_time >= hpo.next_hardfork_time)            
               apply_hardfork(hpo, hpo.current_hardfork + 1);  
      }  
}

BOOST_AUTO_TEST_SUITE(hardfork_tests)

BOOST_AUTO_TEST_CASE (test_has_hardfork){
   // simulate genesis operations
   // during genesis, the STEEMIT_GENESIS_TIME is pushed into processed_hardfork
   new_hardfork_property_object hpo;
   
   BOOST_REQUIRE(has_hardfork(hpo, 0));
   BOOST_REQUIRE(!has_hardfork(hpo, 1));
   BOOST_REQUIRE(!has_hardfork(hpo, 10));
   BOOST_REQUIRE(!has_hardfork(hpo, STEEMIT_NUM_HARDFORKS));
   BOOST_REQUIRE(!has_hardfork(hpo, STEEMIT_NUM_HARDFORKS + 1));

   hpo.current_hardfork = 1;
   BOOST_REQUIRE(has_hardfork(hpo, 0));
   BOOST_REQUIRE(has_hardfork(hpo, 1));
   BOOST_REQUIRE(!has_hardfork(hpo, 10));
   BOOST_REQUIRE(!has_hardfork(hpo, STEEMIT_NUM_HARDFORKS));
   BOOST_REQUIRE(!has_hardfork(hpo, STEEMIT_NUM_HARDFORKS + 1));

   hpo.current_hardfork = 10;
   BOOST_REQUIRE(has_hardfork(hpo, 0));
   BOOST_REQUIRE(has_hardfork(hpo, 1));
   BOOST_REQUIRE(has_hardfork(hpo, 10));
   BOOST_REQUIRE(!has_hardfork(hpo, STEEMIT_NUM_HARDFORKS));
   BOOST_REQUIRE(!has_hardfork(hpo, STEEMIT_NUM_HARDFORKS  +1));

   hpo.current_hardfork = 22;
   BOOST_REQUIRE(has_hardfork(hpo, 0));
   BOOST_REQUIRE(has_hardfork(hpo, 1));
   BOOST_REQUIRE(has_hardfork(hpo, 10));
   BOOST_REQUIRE(has_hardfork(hpo, STEEMIT_NUM_HARDFORKS));
   BOOST_REQUIRE(!has_hardfork(hpo, STEEMIT_NUM_HARDFORKS + 1));
}

BOOST_AUTO_TEST_CASE (test_process_hardforks){
   // scenario: on genesis   
   // hpo.current_hardfork is set to 0 by default.
   // hpo.head_block_time is set to STEEMIT_GENESIS_TIME by default.
   // during init_genesis, the process_hardforks should be never invked.
   new_hardfork_property_object hpo;
   process_hardforks(hpo);
   BOOST_REQUIRE(hpo.current_hardfork == 0);

   // scenario: on block #1, should process through hardfork 19
   hpo.head_block_time += 3;
   process_hardforks(hpo);
   BOOST_REQUIRE(hpo.current_hardfork == 19);

   // scenario: hardfork 20
   hpo.head_block_time = time_point_sec(STEEMIT_HARDFORK_0_20_TIME);
   process_hardforks(hpo);
   BOOST_REQUIRE(hpo.current_hardfork == 20);

   // scenario: hardfork 21
   hpo.head_block_time = time_point_sec(STEEMIT_HARDFORK_0_21_TIME);
   process_hardforks(hpo);
   BOOST_REQUIRE(hpo.current_hardfork == 21);

   // scenario: hardfork 22, just update time should not trigger the process.
   hpo.head_block_time = time_point_sec(STEEMIT_HARDFORK_0_22_TIME);
   process_hardforks(hpo);
   BOOST_REQUIRE(hpo.current_hardfork != 22);

   // scenario: hardfork 22, just update next_hardfork should not trigger the process.
   hpo.head_block_time = time_point_sec(STEEMIT_HARDFORK_0_22_TIME);
   hpo.next_hardfork = STEEMIT_HARDFORK_0_22;  
   process_hardforks(hpo);
   BOOST_REQUIRE(hpo.current_hardfork != 22);

   // scenario: hardfork 22
   hpo.head_block_time = time_point_sec(STEEMIT_HARDFORK_0_22_TIME);
   hpo.next_hardfork = STEEMIT_HARDFORK_0_22;  
   hpo.next_hardfork_time =  time_point_sec(STEEMIT_HARDFORK_0_22_TIME);
   process_hardforks(hpo);
   BOOST_REQUIRE(hpo.current_hardfork == 22);
}

BOOST_AUTO_TEST_SUITE_END()

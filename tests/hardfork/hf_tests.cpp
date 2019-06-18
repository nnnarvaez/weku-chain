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

struct hardfork_witness_object{
   uint32_t next_hardfork_vote = 0;
   time_point_sec next_hardfork_time_vote = time_point_sec::maximum();
};

std::map<std::pair<uint32_t, time_point_sec>, uint32_t> get_hardfork_votes(const std::vector<hardfork_witness_object>& witnesses){
   std::map<std::pair<uint32_t, time_point_sec>, uint32_t> votes;
   for(const auto& w : witnesses){
      auto vote = std::make_pair(w.next_hardfork_vote, w.next_hardfork_time_vote);
      if(votes.find(vote) == votes.end())
         votes[vote] = 1;
      else
         votes[vote] += 1;
   }

   return votes;
}

void update_majority_hardforks(const std::vector<hardfork_witness_object>& witnesses, hardfork_property_object& hpo){
   std::map<std::pair<uint32_t, time_point_sec>, uint32_t> hardfork_votes = get_hardfork_votes(witnesses);

   uint32_t required_witnesses = 17;
   
   for(const auto& v : hardfork_votes){
      if(v.second >= required_witnesses){ // found majority
         hpo.next_hardfork = std::get<0>(v.first);
         hpo.next_hardfork_time = std::get<1>(v.first);
         break;
      }
   }
}

void helper_push_hardfork_witness_object(std::vector<hardfork_witness_object> &witnesses,
   uint32_t hardfork_vote, time_point_sec hardfork_time_vote){
   hardfork_witness_object w;
   w.next_hardfork_vote = hardfork_vote;
   w.next_hardfork_time_vote = hardfork_time_vote;
   witnesses.push_back(w);
}

BOOST_AUTO_TEST_SUITE(hardfork_tests)

BOOST_AUTO_TEST_CASE (test_process_hardforks){
   // scenario: on genesis   
   // hpo.current_hardfork is set to 0 by default.
   // hpo.head_block_time is set to STEEMIT_GENESIS_TIME by default.
   // during init_genesis, the process_hardforks should be never invked.
   hardfork_property_object hpo;
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

   // scenario: hardfork 22, should only apply once.
   hpo.head_block_time = time_point_sec(STEEMIT_HARDFORK_0_22_TIME);
   hpo.next_hardfork = STEEMIT_HARDFORK_0_22;  
   hpo.next_hardfork_time =  time_point_sec(STEEMIT_HARDFORK_0_22_TIME);
   process_hardforks(hpo);
   BOOST_REQUIRE(hpo.current_hardfork == 22);
   BOOST_TEST_MESSAGE("hardfork 22 only applied once.");
}

BOOST_AUTO_TEST_CASE (test_update_majority_hardforks){

   hardfork_property_object hpo;
   hpo.current_hardfork = STEEMIT_HARDFORK_0_21;
   // simulate next blockafter hardfork 21
   hpo.head_block_time = time_point_sec(STEEMIT_HARDFORK_0_21_TIME) + 3;    
   if(hpo.next_hardfork == 0){ // as a reminder to set after hf21
      hpo.next_hardfork = STEEMIT_HARDFORK_0_21;
      hpo.next_hardfork_time = time_point_sec(STEEMIT_HARDFORK_0_21_TIME);
   }

   std::vector<hardfork_witness_object> witnesses;
   for(int i = 0; i < 16; i++){
      helper_push_hardfork_witness_object(witnesses, STEEMIT_HARDFORK_0_22, time_point_sec(STEEMIT_HARDFORK_0_22_TIME));
      update_majority_hardforks(witnesses, hpo);
      BOOST_REQUIRE(hpo.next_hardfork == STEEMIT_HARDFORK_0_21);
      BOOST_REQUIRE(hpo.next_hardfork_time == time_point_sec(STEEMIT_HARDFORK_0_21_TIME));
   }

   // reach 17, next hardfork condition satisfied.
   helper_push_hardfork_witness_object(witnesses, STEEMIT_HARDFORK_0_22, time_point_sec(STEEMIT_HARDFORK_0_22_TIME));
   update_majority_hardforks(witnesses, hpo);
   BOOST_REQUIRE(hpo.next_hardfork == STEEMIT_HARDFORK_0_22);
   BOOST_REQUIRE(hpo.next_hardfork_time == time_point_sec(STEEMIT_HARDFORK_0_22_TIME));

}

BOOST_AUTO_TEST_CASE(){test_process_hardfork2){
   hardforker hr();
   

}

BOOST_AUTO_TEST_SUITE_END()

/* 
before hf22 (not include hf22), system uses current_hardfork and hardfork_times to trigger hardfork
after hf22( include hf22), system uses current_hardfork and next_hardfork to trigger hardfork.
bool should_hardfork_triggered(const hardfork_property_object& hpo){
   if(hardforker::has_hardfork(hpo, STEEMIT_HARDFORK_0_21))      
      return hpo.current_hardfork < hpo.next_hardfork 
         && hpo.head_block_time >= hpo.next_hardfork_time;
   else
      return hpo.current_hardfork < STEEMIT_HARDFORK_0_21 
         // _hardfork_times[ hpo.current_hardfork + 1 ] means next_hardfork_time
         && hpo.head_block_time >= hardfork_property_object::hardfork_times[ hpo.current_hardfork + 1 ]; 
}
*/

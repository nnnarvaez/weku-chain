#pragma once

#include<steemit/protocol/config.hpp>

namespace steemit{namespace chain {
class database;

std::vector<fc::time_point_sec> hardfork_times{
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

class hardforker{
   private:
      database& _db;

   public:
   hardforker(database& db):_db(db){}

   void process_hardforks(hardfork_property_object& hpo)
   void apply_hardfork(hardfork_property_object& hpo, uint32_t hardfork);

   void perform_vesting_share_split( uint32_t magnitude );
   void retally_witness_votes();
   void retally_witness_vote_counts( bool force = false );
   void retally_comment_children();  
   void do_hardfork_9();
   void retally_liquidity_weight();
   void do_hardfork_12();
   void do_hardfork_16();
   void do_hardfork_17();
   void do_hardfork_19();
   void do_hardfork_21();
   void do_hardfork_22();

};


}}
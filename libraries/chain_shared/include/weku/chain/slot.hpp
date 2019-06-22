#pragma once

#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{

class slot{
    public:
        slot(const itemp_database& db):_db(db){}
        /**
         * Get the time at which the given slot occurs.
         *
         * If slot_num == 0, return time_point_sec().
         *
         * If slot_num == N for N > 0, return the Nth next
         * block-interval-aligned time greater than head_block_time().
         */
        fc::time_point_sec get_slot_time(uint32_t slot_num)const;
        
        /**
          * Get the last slot which occurs AT or BEFORE the given time.
          *
          * The return value is the greatest value N such that
          * get_slot_time( N ) <= when.
          *
          * If no such N exists, return 0.
          */
        uint32_t get_slot_at_time(fc::time_point_sec when)const;
    private:
        const itemp_database& _db;
};

}}
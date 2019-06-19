#pragma once

#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{

class slot{
    public:
        slot(const itemp_database& db):_db(db){}
        fc::time_point_sec get_slot_time(uint32_t slot_num)const;
        uint32_t get_slot_at_time(fc::time_point_sec when)const;
    private:
        const itemp_database& _db;
};

}}
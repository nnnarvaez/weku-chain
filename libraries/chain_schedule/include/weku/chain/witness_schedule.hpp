#pragma once
#include<weku/chain/itemp_database.hpp>

namespace weku { namespace chain {

void update_witness_schedule( itemp_database& db );
void reset_virtual_schedule_time( itemp_database& db );

} }

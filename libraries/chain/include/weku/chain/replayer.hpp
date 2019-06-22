#pragma once
#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{

class replayer{
public:
    replayer(itemp_database& db):_db(db){}
    void reindex( const fc::path& data_dir, const fc::path& shared_mem_dir, uint64_t shared_file_size )
private:
    itemp_database& _db;
};

}}
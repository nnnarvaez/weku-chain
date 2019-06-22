#pragma once
#include <chainbase/object.hpp>

namespace chainbase{

#ifndef CHAINBASE_NUM_RW_LOCKS
   #define CHAINBASE_NUM_RW_LOCKS 10
#endif

typedef boost::interprocess::interprocess_sharable_mutex read_write_mutex;
typedef boost::interprocess::sharable_lock< read_write_mutex > read_lock;
typedef boost::unique_lock< read_write_mutex > write_lock;

class read_write_mutex_manager
{
    public:
        read_write_mutex_manager()
        {
        _current_lock = 0;
        }

        ~read_write_mutex_manager(){}

        void next_lock()
        {
        _current_lock++;
        new( &_locks[ _current_lock % CHAINBASE_NUM_RW_LOCKS ] ) read_write_mutex();
        }

        read_write_mutex& current_lock()
        {
        return _locks[ _current_lock % CHAINBASE_NUM_RW_LOCKS ];
        }

        uint32_t current_lock_num()
        {
        return _current_lock;
        }

    private:
        std::array< read_write_mutex, CHAINBASE_NUM_RW_LOCKS >     _locks;
        std::atomic< uint32_t >                                    _current_lock;
};

}
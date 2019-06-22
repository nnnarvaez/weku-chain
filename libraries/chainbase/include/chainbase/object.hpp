#pragma once
#include <cstdint>
#include <vector>
#include<memory>

#include <boost/filesystem.hpp>

namespace chainbase{
namespace bip = boost::interprocess;
namespace bfs = boost::filesystem;
using std::unique_ptr;
using std::vector;

template<typename T>
using allocator = bip::allocator<T, bip::managed_mapped_file::segment_manager>;

typedef bip::basic_string< char, std::char_traits< char >, allocator< char > > shared_string;

template<typename T>
using shared_vector = std::vector<T, allocator<T> >;


/**
* The code we want to implement is this:
*
* ++target; try { ... } finally { --target }
*
* In C++ the only way to implement finally is to create a class
* with a destructor, so that's what we do here.
*/
class int_incrementer
{
    public:
        int_incrementer( int32_t& target ) : _target(target)
        { ++_target; }
        ~int_incrementer()
        { --_target; }

        int32_t get()const
        { return _target; }

    private:
        int32_t& _target;
};

struct strcmp_less
{
    bool operator()( const shared_string& a, const shared_string& b )const
    {
        return less( a.c_str(), b.c_str() );
    }

    bool operator()( const shared_string& a, const std::string& b )const
    {
        return less( a.c_str(), b.c_str() );
    }

    bool operator()( const std::string& a, const shared_string& b )const
    {
        return less( a.c_str(), b.c_str() );
    }
    private:
        inline bool less( const char* a, const char* b )const
        {
        return std::strcmp( a, b ) < 0;
        }
};

/**
*  Object ID type that includes the type of the object it references
*/
template<typename T>
class oid {
    public:
        oid( int64_t i = 0 ):_id(i){}

        oid& operator++() { ++_id; return *this; }

        friend bool operator < ( const oid& a, const oid& b ) { return a._id < b._id; }
        friend bool operator > ( const oid& a, const oid& b ) { return a._id > b._id; }
        friend bool operator == ( const oid& a, const oid& b ) { return a._id == b._id; }
        friend bool operator != ( const oid& a, const oid& b ) { return a._id != b._id; }
        int64_t _id = 0;
};

template<uint16_t TypeNumber, typename Derived>
struct object
{
    typedef oid<Derived> id_type;
    static const uint16_t type_id = TypeNumber;

};

#define CHAINBASE_DEFAULT_CONSTRUCTOR( OBJECT_TYPE ) \
template<typename Constructor, typename Allocator> \
OBJECT_TYPE( Constructor&& c, Allocator&&  ) { c(*this); }

/** this class is ment to be specified to enable lookup of index type by object type using
* the SET_INDEX_TYPE macro.
**/
template<typename T>
struct get_index_type {};

/**
*  This macro must be used at global scope and OBJECT_TYPE and INDEX_TYPE must be fully qualified
*/
#define CHAINBASE_SET_INDEX_TYPE( OBJECT_TYPE, INDEX_TYPE )  \
namespace chainbase { template<> struct get_index_type<OBJECT_TYPE> { typedef INDEX_TYPE type; }; }



}
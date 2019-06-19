#pragma once

#include <steemit/protocol/types.hpp>

#include <fc/uint128.hpp>

namespace weku { namespace chain { namespace util {

inline steemit::u256 to256( const fc::uint128& t )
{
   steemit::u256 v(t.hi);
   v <<= 64;
   v += t.lo;
   return v;
}

} } }

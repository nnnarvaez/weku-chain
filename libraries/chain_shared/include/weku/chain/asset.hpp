#pragma once

#include <weku/protocol/asset.hpp>
#include <weku/chain/itemp_database.hpp>

namespace weku { namespace chain { namespace util {

using weku::protocol::asset;
using weku::protocol::price;

/**
 * Helper method to return the current sbd value of a given amount of
 * STEEM.  Return 0 SBD if there isn't a current_median_history
 */
inline asset to_sbd(const weku::chain::itemp_database& db, const asset& steem )
{
   return to_sbd( db.get_feed_history().current_median_history, steem );
}

inline asset to_sbd( const price& p, const asset& steem )
{
   FC_ASSERT( steem.symbol == STEEM_SYMBOL );
   if( p.is_null() )
      return asset( 0, SBD_SYMBOL );
   return steem * p;
}

inline asset to_steem( const price& p, const asset& sbd )
{
   FC_ASSERT( sbd.symbol == SBD_SYMBOL );
   if( p.is_null() )
      return asset( 0, STEEM_SYMBOL );
   return sbd * p;
}

} } }

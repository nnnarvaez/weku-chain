#include <wk/chain_refactory/conversion_processor.hpp>

using namespace steemit::chain;

namespace wk{namespace chain{

/**
 *  Iterates over all conversion requests with a conversion date before
 *  the head block time and then converts them to/from steem/sbd at the
 *  current median price feed history price times the premium
 */
void conversion_processor::process_conversions()
{
   auto now = _db.head_block_time();
   const auto& request_by_date = _db.get_index< convert_request_index >().indices().get< by_conversion_date >();
   auto itr = request_by_date.begin();

   const auto& fhistory = _db.get_feed_history();
   if( fhistory.current_median_history.is_null() )
      return;

   asset net_sbd( 0, SBD_SYMBOL );
   asset net_steem( 0, STEEM_SYMBOL );

   while( itr != request_by_date.end() && itr->conversion_date <= now )
   {
      const auto& user = _db.get_account( itr->owner );
      auto amount_to_issue = itr->amount * fhistory.current_median_history;

      _db.adjust_balance( user, amount_to_issue );

      net_sbd   += itr->amount;
      net_steem += amount_to_issue;

      _db.push_virtual_operation( fill_convert_request_operation ( user.name, itr->requestid, itr->amount, amount_to_issue ) );

      _db.remove( *itr );
      itr = request_by_date.begin();
   }

   const auto& props = _db.get_dynamic_global_properties();
   _db.modify( props, [&]( dynamic_global_property_object& p )
   {
       p.current_supply += net_steem;
       p.current_sbd_supply -= net_sbd;
       p.virtual_supply += net_steem;
       p.virtual_supply -= net_sbd * _db.get_feed_history().current_median_history;
   } );
}

}}
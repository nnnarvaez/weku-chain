#include <weku/chain/account_recovery_processor.hpp>

namespace weku{ namespace chain {

void account_recovery_processor::account_recovery_processing()
{
   // Clear expired recovery requests
   const auto& rec_req_idx = _db.get_index< account_recovery_request_index >().indices().get< by_expiration >();
   auto rec_req = rec_req_idx.begin();

   while( rec_req != rec_req_idx.end() && rec_req->expires <= _db.head_block_time() )
   {
      _db.remove( *rec_req );
      rec_req = rec_req_idx.begin();
   }

   // Clear invalid historical authorities
   const auto& hist_idx = _db.get_index< owner_authority_history_index >().indices(); //by id
   auto hist = hist_idx.begin();

   while( hist != hist_idx.end() && fc::time_point_sec( hist->last_valid_time + STEEMIT_OWNER_AUTH_RECOVERY_PERIOD ) < _db.head_block_time() )
   {
      _db.remove( *hist );
      hist = hist_idx.begin();
   }

   // Apply effective recovery_account changes
   const auto& change_req_idx = _db.get_index< change_recovery_account_request_index >().indices().get< by_effective_date >();
   auto change_req = change_req_idx.begin();

   while( change_req != change_req_idx.end() && change_req->effective_on <= _db.head_block_time() )
   {
      _db.modify( _db.get_account( change_req->account_to_recover ), [&]( account_object& a )
      {
         a.recovery_account = change_req->recovery_account;
      });

      _db.remove( *change_req );
      change_req = change_req_idx.begin();
   }
}



}}
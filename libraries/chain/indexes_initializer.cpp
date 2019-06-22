
#include <weku/chain/indexes_initializer.hpp>
#include <weku/chain/index.hpp>
#include <weku/chain/operation_object.hpp>
#include <weku/chain/blacklist_objects.hpp>

namespace weku{namespace chain{
void indexes_initializer::initialize_indexes()
{
   add_core_index< dynamic_global_property_index           >(_db);
   add_core_index< account_index                           >(_db);
   add_core_index< account_authority_index                 >(_db);
   add_core_index< witness_index                           >(_db);
   add_core_index< transaction_index                       >(_db);
   add_core_index< block_summary_index                     >(_db);
   add_core_index< witness_schedule_index                  >(_db);
   add_core_index< comment_index                           >(_db);
   add_core_index< comment_vote_index                      >(_db);
   add_core_index< witness_vote_index                      >(_db);
   add_core_index< limit_order_index                       >(_db);
   add_core_index< feed_history_index                      >(_db);
   add_core_index< convert_request_index                   >(_db);
   add_core_index< liquidity_reward_balance_index          >(_db);
   add_core_index< operation_index                         >(_db);
   add_core_index< account_history_index                   >(_db);
   add_core_index< hardfork_property_index                 >(_db);
   add_core_index< withdraw_vesting_route_index            >(_db);
   add_core_index< owner_authority_history_index           >(_db);
   add_core_index< account_recovery_request_index          >(_db);
   add_core_index< change_recovery_account_request_index   >(_db);
   add_core_index< escrow_index                            >(_db);
   add_core_index< savings_withdraw_index                  >(_db);
   add_core_index< decline_voting_rights_request_index     >(_db);
   add_core_index< reward_fund_index                       >(_db);
   add_core_index< vesting_delegation_index                >(_db);
   add_core_index< vesting_delegation_expiration_index     >(_db);
   add_core_index< blacklist_vote_index                    >(_db);
}
}}
#include <weku/chain/evaluators_initializer.hpp>
#include <weku/chain/evaluator.hpp>

namespace weku{namespace chain{

// add evaluators into evaluator_registory's internal list
void evaluators_initializer::initialize_evaluators()
{
   _registry.register_evaluator< vote_evaluator                           >();
   _registry.register_evaluator< comment_evaluator                        >();
   _registry.register_evaluator< comment_options_evaluator                >();
   _registry.register_evaluator< delete_comment_evaluator                 >();
   _registry.register_evaluator< transfer_evaluator                       >();
   _registry.register_evaluator< transfer_to_vesting_evaluator            >();
   _registry.register_evaluator< withdraw_vesting_evaluator               >();
   _registry.register_evaluator< set_withdraw_vesting_route_evaluator     >();
   _registry.register_evaluator< account_create_evaluator                 >();
   _registry.register_evaluator< account_update_evaluator                 >();
   _registry.register_evaluator< witness_update_evaluator                 >();
   _registry.register_evaluator< account_witness_vote_evaluator           >();
   _registry.register_evaluator< account_witness_proxy_evaluator          >();
   _registry.register_evaluator< custom_evaluator                         >();
   _registry.register_evaluator< custom_binary_evaluator                  >();
   _registry.register_evaluator< custom_json_evaluator                    >();
   _registry.register_evaluator< pow_evaluator                            >();
   _registry.register_evaluator< pow2_evaluator                           >();
   _registry.register_evaluator< report_over_production_evaluator         >();
   _registry.register_evaluator< feed_publish_evaluator                   >();
   _registry.register_evaluator< convert_evaluator                        >();
   _registry.register_evaluator< limit_order_create_evaluator             >();
   _registry.register_evaluator< limit_order_create2_evaluator            >();
   _registry.register_evaluator< limit_order_cancel_evaluator             >();
   _registry.register_evaluator< challenge_authority_evaluator            >();
   _registry.register_evaluator< prove_authority_evaluator                >();
   _registry.register_evaluator< request_account_recovery_evaluator       >();
   _registry.register_evaluator< recover_account_evaluator                >();
   _registry.register_evaluator< change_recovery_account_evaluator        >();
   _registry.register_evaluator< escrow_transfer_evaluator                >();
   _registry.register_evaluator< escrow_approve_evaluator                 >();
   _registry.register_evaluator< escrow_dispute_evaluator                 >();
   _registry.register_evaluator< escrow_release_evaluator                 >();
   _registry.register_evaluator< transfer_to_savings_evaluator            >();
   _registry.register_evaluator< transfer_from_savings_evaluator          >();
   _registry.register_evaluator< cancel_transfer_from_savings_evaluator   >();
   _registry.register_evaluator< decline_voting_rights_evaluator          >();
   _registry.register_evaluator< reset_account_evaluator                  >();
   _registry.register_evaluator< set_reset_account_evaluator              >();
   _registry.register_evaluator< claim_reward_balance_evaluator           >();
   _registry.register_evaluator< account_create_with_delegation_evaluator >();
   _registry.register_evaluator< delegate_vesting_shares_evaluator        >();
   _registry.register_evaluator< blacklist_vote_evaluator                 >();
}
}}
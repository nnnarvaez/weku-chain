#pragma once

#include <chainbase/chainbase.hpp>
#include <steemit/chain/steem_object_types.hpp>
#include <steemit/protocol/block.hpp>
#include <steemit/chain/account_object.hpp>
#include <steemit/protocol/steem_operations.hpp>
#include <steemit/protocol/config.hpp>
#include <steemit/chain/global_property_object.hpp>
#include <steemit/chain/comment_object.hpp>
#include <steemit/chain/steem_objects.hpp>
#include <fc/uint128.hpp>
#include <steemit/chain/witness_schedule.hpp>

#include <wk/chain_refactory/hardfork_constants.hpp>

// #include <steemit/chain/block_summary_object.hpp>
// #include <steemit/chain/compound.hpp>
// #include <steemit/chain/custom_operation_interpreter.hpp>
// 
// #include <steemit/chain/history_object.hpp>
// #include <steemit/chain/index.hpp>
// #include <steemit/chain/transaction_object.hpp>
// #include <steemit/chain/shared_db_merkle.hpp>
// #include <steemit/chain/operation_notification.hpp>
// 

// #include <steemit/chain/util/asset.hpp>
// #include <steemit/chain/util/reward.hpp>
// #include <steemit/chain/util/uint256.hpp>
// #include <steemit/chain/util/reward.hpp>

using namespace steemit::protocol;
using namespace steemit::chain;

namespace wk{ namespace chain{

// TODO: need to find hardfork numbers
#define HARDFORK_20_BLOCK_NUM 100
#define HARDFORK_21_BLOCK_NUM 200
#define HARDFORK_22_BLOCK_NUM_FROM 15000000u

typedef std::vector<std::pair<uint32_t, uint32_t> > hardfork_votes_type;

enum validation_steps
    {
    skip_nothing                = 0,
    skip_witness_signature      = 1 << 0,  ///< used while reindexing
    skip_transaction_signatures = 1 << 1,  ///< used by non-witness nodes
    skip_transaction_dupe_check = 1 << 2,  ///< used while reindexing
    skip_fork_db                = 1 << 3,  ///< used while reindexing
    skip_block_size_check       = 1 << 4,  ///< used when applying locally generated transactions
    skip_tapos_check            = 1 << 5,  ///< used while reindexing -- note this skips expiration check as well
    skip_authority_check        = 1 << 6,  ///< used while reindexing -- disables any checking of authority on transactions
    skip_merkle_check           = 1 << 7,  ///< used while reindexing
    skip_undo_history_check     = 1 << 8,  ///< used while reindexing
    skip_witness_schedule_check = 1 << 9,  ///< used while reindexing
    skip_validate               = 1 << 10, ///< used prior to checkpoint, skips validate() call on transaction
    skip_validate_invariants    = 1 << 11, ///< used to skip database invariant check on block application
    skip_undo_block             = 1 << 12, ///< used to skip undo db on reindex
    skip_block_log              = 1 << 13  ///< used to skip block logging on reindex
};

// this class is to tempararily used to refactory database
class itemp_database: public chainbase::database 
{
    public:
    virtual ~itemp_database() = default;
    virtual void init_genesis(uint64_t initial_supply);

    virtual uint32_t head_block_num()const;
    virtual uint32_t last_hardfork();
    virtual void last_hardfork(uint32_t hardfork);  
    virtual bool has_hardfork( uint32_t hardfork )const;

    virtual hardfork_votes_type next_hardfork_votes();
    virtual void next_hardfork_votes(hardfork_votes_type next_hardfork_votes);

    virtual const dynamic_global_property_object&  get_dynamic_global_properties()const;
    virtual const witness_schedule_object&         get_witness_schedule_object()const;
    virtual const feed_history_object&             get_feed_history()const;

    virtual const account_object&  get_account(  const account_name_type& name )const;
    virtual const account_object*  find_account( const account_name_type& name )const;

    virtual void apply_block( const signed_block& next_block, uint32_t skip = skip_nothing );

    virtual fc::time_point_sec   head_block_time()const;
    virtual void        adjust_balance( const account_object& a, const asset& delta );
    virtual void adjust_witness_votes( const account_object& a, share_type delta );
    virtual void adjust_witness_vote( const witness_object& obj, share_type delta );
    virtual void adjust_rshares2( const comment_object& comment, fc::uint128_t old_rshares2, fc::uint128_t new_rshares2 );
    virtual void adjust_proxied_witness_votes( const account_object& a,
                                            const std::array< share_type, STEEMIT_MAX_PROXY_RECURSION_DEPTH+1 >& delta,
                                            int depth = 0 );
    virtual void adjust_liquidity_reward( const account_object& owner, const asset& volume, bool is_bid );
    virtual const void push_virtual_operation( const operation& op, bool force = false ); 
    
    virtual void cancel_order( const limit_order_object& obj );

    virtual const reward_fund_object& get_reward_fund( const comment_object& c )const;
    virtual uint16_t get_curation_rewards_percent( const comment_object& c ) const;
    virtual share_type pay_curators( const comment_object& c, share_type& max_rewards );
    virtual asset create_vesting( const account_object& to_account, asset steem, bool to_reward_balance=false );
    virtual std::pair< asset, asset > create_sbd( const account_object& to_account, asset steem, bool to_reward_balance=false );
    
    virtual void adjust_total_payout( const comment_object& a, const asset& sbd, const asset& curator_sbd_value, const asset& beneficiary_value );
    virtual void validate_invariants()const;
    virtual const time_point_sec                   calculate_discussion_payout_time( const comment_object& comment )const;
};

}}
#include "gmock/gmock.h"

#include <weku/chain/i_hardfork_doer.hpp>
#include <weku/chain/hardforker.hpp>

namespace weku{namespace chain{

class mock_database: public itemp_database
{
    public:
    MOCK_METHOD0(last_hardfork, uint32_t());
    virtual void init_genesis(uint64_t initial_supply) = 0;

    virtual uint32_t head_block_num()const = 0;
    virtual uint32_t last_hardfork() const = 0;
    virtual void last_hardfork(uint32_t hardfork) = 0;  
    virtual bool has_hardfork( uint32_t hardfork )const = 0;

    virtual hardfork_votes_type next_hardfork_votes() const = 0;
    virtual void next_hardfork_votes(hardfork_votes_type next_hardfork_votes) = 0;

    //virtual signed_block generate_block_before_apply(); 

    virtual const dynamic_global_property_object&  get_dynamic_global_properties()const = 0;
    virtual const witness_schedule_object&         get_witness_schedule_object()const = 0;
    virtual const hardfork_property_object&        get_hardfork_property_object()const = 0;
    virtual const feed_history_object&             get_feed_history()const = 0;
    virtual account_name_type get_scheduled_witness(uint32_t slot_num)const = 0;

    virtual block_id_type head_block_id()const = 0;
    virtual fc::time_point_sec head_block_time()const = 0;

    
    virtual const account_object&  get_account(  const account_name_type& name )const = 0;
    virtual const account_object*  find_account( const account_name_type& name )const = 0; 
    virtual const witness_object&  get_witness(  const account_name_type& name )const = 0;
    virtual const witness_object*  find_witness( const account_name_type& name )const = 0;

    virtual void apply_block( const signed_block& next_block, uint32_t skip ) = 0;

    virtual void adjust_savings_balance( const account_object& a, const asset& delta ) = 0; // move
    virtual void adjust_reward_balance( const account_object& a, const asset& delta ) = 0; // move
    virtual void adjust_balance( const account_object& a, const asset& delta ) = 0;
    virtual void adjust_supply( const asset& delta, bool adjust_vesting = false ) = 0; // move
    virtual void adjust_witness_votes( const account_object& a, share_type delta ) = 0;
    virtual void adjust_witness_vote( const witness_object& obj, share_type delta ) = 0;
    
    virtual void adjust_proxied_witness_votes( const account_object& a,
                                            const std::array< share_type, STEEMIT_MAX_PROXY_RECURSION_DEPTH+1 >& delta,
                                            int depth = 0) = 0;
    virtual void adjust_proxied_witness_votes( const account_object& a, share_type delta, int depth = 0) = 0;
    virtual const void push_virtual_operation( const operation& op, bool force = false ) = 0; 
    
    virtual const reward_fund_object& get_reward_fund( const comment_object& c )const = 0;
    
    virtual asset create_vesting( const account_object& to_account, asset steem, bool to_reward_balance = false) = 0;
    
    
    virtual asset to_sbd( const asset& steem )const = 0;

    virtual void validate_invariants()const = 0; // move
    
    
    
    virtual const node_property_object& get_node_properties()const = 0;
    virtual node_property_object& node_properties() = 0;

    virtual block_log& get_block_log() = 0; 
    virtual fork_database& fork_db() = 0;

    
    virtual bool is_producing() const = 0;
   
    virtual const comment_object&  get_comment(  const account_name_type& author, const shared_string& permlink )const = 0;
    virtual const comment_object*  find_comment( const account_name_type& author, const shared_string& permlink )const = 0;

    virtual const comment_object&  get_comment(  const account_name_type& author, const string& permlink )const = 0;
    virtual const comment_object*  find_comment( const account_name_type& author, const string& permlink )const = 0;

    virtual const escrow_object&   get_escrow(  const account_name_type& name, uint32_t escrow_id )const = 0;
    
    virtual std::shared_ptr< custom_operation_interpreter > get_custom_json_evaluator( const std::string& id ) = 0;
    
};

class mock_hardfork_doer: public i_hardfork_doer{
public:

};

}}

using namespace weku::chain;

TEST(hardfork_19, should_be_triggerred_at_block_1){
    mock_database db;
    mock_hardfork_doer doer;    
    hardforker hfkr(db, dore);
}

// TEST(hardfork_22, should_not_trigger_without_enough_votes){
//     mock_database db;
//     hardforker hfr(db);
// }
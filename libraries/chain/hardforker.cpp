#include<steemit/chain/hardforker.hpp>

namespace steemit{namespace chain {

// for hardfork 1
void hardforker::perform_vesting_share_split( uint32_t magnitude )
{
   try
   {
      _db.modify( _db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& d )
      {
         d.total_vesting_shares.amount *= magnitude;
         // will be adjusted at bottom by function: adjust_rshares2
         d.total_reward_shares2 = 0;
      } );

      // Need to update all VESTS in accounts and the total VESTS in the dgpo
      for( const auto& account : get_index<account_index>().indices() )
      {
         modify( account, [&]( account_object& a )
         {
            a.vesting_shares.amount *= magnitude;
            a.withdrawn             *= magnitude;
            a.to_withdraw           *= magnitude;
            a.vesting_withdraw_rate  = asset( a.to_withdraw / STEEMIT_VESTING_WITHDRAW_INTERVALS_PRE_HF_16, VESTS_SYMBOL );
            if( a.vesting_withdraw_rate.amount == 0 )
               a.vesting_withdraw_rate.amount = 1;

            for( uint32_t i = 0; i < STEEMIT_MAX_PROXY_RECURSION_DEPTH; ++i )
               a.proxied_vsf_votes[i] *= magnitude;
         } );
      }

      const auto& comments = get_index< comment_index >().indices();
      for( const auto& comment : comments )
      {
         modify( comment, [&]( comment_object& c )
         {
            c.net_rshares       *= magnitude;
            c.abs_rshares       *= magnitude;
            c.vote_rshares      *= magnitude;
         } );
      }

      for( const auto& c : comments )
      {
         if( c.net_rshares.value > 0 )
            adjust_rshares2( c, 0, util::evaluate_reward_curve( c.net_rshares.value ) );
      }

   }
   FC_CAPTURE_AND_RETHROW()
}

// for hardfork 2 and 3
void hardforker::retally_witness_votes()
{
   const auto& witness_idx = get_index< witness_index >().indices();

   // Clear all witness votes
   for( auto itr = witness_idx.begin(); itr != witness_idx.end(); ++itr )
   {
      modify( *itr, [&]( witness_object& w )
      {
         w.votes = 0;
         w.virtual_position = 0;
      } );
   }

   const auto& account_idx = get_index< account_index >().indices();

   // Apply all existing votes by account
   for( auto itr = account_idx.begin(); itr != account_idx.end(); ++itr )
   {
      if( itr->proxy != STEEMIT_PROXY_TO_SELF_ACCOUNT ) continue;

      const auto& a = *itr;

      const auto& vidx = get_index<witness_vote_index>().indices().get<by_account_witness>();
      auto wit_itr = vidx.lower_bound( boost::make_tuple( a.id, witness_id_type() ) );
      while( wit_itr != vidx.end() && wit_itr->account == a.id )
      {
         adjust_witness_vote( get(wit_itr->witness), a.witness_vote_weight() );
         ++wit_itr;
      }
   }
}

// for hardfork 6 and 8
void hardforker::retally_witness_vote_counts( bool force )
{
   const auto& account_idx = get_index< account_index >().indices();

   // Check all existing votes by account
   for( auto itr = account_idx.begin(); itr != account_idx.end(); ++itr )
   {
      const auto& a = *itr;
      uint16_t witnesses_voted_for = 0;
      if( force || (a.proxy != STEEMIT_PROXY_TO_SELF_ACCOUNT  ) )
      {
        const auto& vidx = get_index< witness_vote_index >().indices().get< by_account_witness >();
        auto wit_itr = vidx.lower_bound( boost::make_tuple( a.id, witness_id_type() ) );
        while( wit_itr != vidx.end() && wit_itr->account == a.id )
        {
           ++witnesses_voted_for;
           ++wit_itr;
        }
      }
      if( a.witnesses_voted_for != witnesses_voted_for )
      {
         modify( a, [&]( account_object& account )
         {
            account.witnesses_voted_for = witnesses_voted_for;
         } );
      }
   }
}

// for hardfork 06
void hardforker::retally_comment_children()
{
   const auto& cidx = get_index< comment_index >().indices();

   // Clear children counts
   for( auto itr = cidx.begin(); itr != cidx.end(); ++itr )
   {
      modify( *itr, [&]( comment_object& c )
      {
         c.children = 0;
      });
   }

   for( auto itr = cidx.begin(); itr != cidx.end(); ++itr )
   {
      if( itr->parent_author != STEEMIT_ROOT_POST_PARENT )
      {
        // Low memory nodes only need immediate child count, full nodes track total children
        #ifdef IS_LOW_MEM
         modify( get_comment( itr->parent_author, itr->parent_permlink ), [&]( comment_object& c )
         {
            c.children++;
         });
        #else
         const comment_object* parent = &get_comment( itr->parent_author, itr->parent_permlink );
         while( parent )
         {
            modify( *parent, [&]( comment_object& c )
            {
               c.children++;
            });

            if( parent->parent_author != STEEMIT_ROOT_POST_PARENT )
               parent = &get_comment( parent->parent_author, parent->parent_permlink );
            else
               parent = nullptr;
         }
        #endif
      }
   }
}

// for hardfork 09
void hardforker::do_hardfork_9(){    
    for( const std::string& acc : hardfork9::get_compromised_accounts() )
    {
        const account_object* account = find_account( acc );
        if( account == nullptr )
            continue;

        update_owner_authority( *account, authority( 1, public_key_type( "STM7sw22HqsXbz7D2CmJfmMwt9rimtk518dRzsR1f8Cgw52dQR1pR" ), 1 ) );

        modify( get< account_authority_object, by_account >( account->name ), [&]( account_authority_object& auth )
        {
            auth.active  = authority( 1, public_key_type( "STM7sw22HqsXbz7D2CmJfmMwt9rimtk518dRzsR1f8Cgw52dQR1pR" ), 1 );
            auth.posting = authority( 1, public_key_type( "STM7sw22HqsXbz7D2CmJfmMwt9rimtk518dRzsR1f8Cgw52dQR1pR" ), 1 );
        });
    }         
}

// for hardfork 10
void hardforker::retally_liquidity_weight() {
   const auto& ridx = get_index< liquidity_reward_balance_index >().indices().get< by_owner >();
   for( const auto& i : ridx ) {
      modify( i, []( liquidity_reward_balance_object& o ){
         o.update_weight(true/*HAS HARDFORK10 if this method is called*/);
      });
   }
}

void hardforker::do_hardfork_12()
{
    const auto& comment_idx = get_index< comment_index >().indices();

    for( auto itr = comment_idx.begin(); itr != comment_idx.end(); ++itr )
    {
        // At the hardfork time, all new posts with no votes get their cashout time set to +12 hrs from head block time.
        // All posts with a payout get their cashout time set to +30 days. This hardfork takes place within 30 days
        // initial payout so we don't have to handle the case of posts that should be frozen that aren't
        if( itr->parent_author == STEEMIT_ROOT_POST_PARENT )
        {
            // Post has not been paid out and has no votes (cashout_time == 0 === net_rshares == 0, under current semmantics)
            if( itr->last_payout == fc::time_point_sec::min() && itr->cashout_time == fc::time_point_sec::maximum() )
            {
                modify( *itr, [&]( comment_object & c )
                {
                c.cashout_time = head_block_time() + STEEMIT_CASHOUT_WINDOW_SECONDS_PRE_HF17;
                });
            }
            // Has been paid out, needs to be on second cashout window
            else if( itr->last_payout > fc::time_point_sec() )
            {
                modify( *itr, [&]( comment_object& c )
                {
                c.cashout_time = c.last_payout + STEEMIT_SECOND_CASHOUT_WINDOW;
                });
            }
        }
    }

    modify( get< account_authority_object, by_account >( STEEMIT_MINER_ACCOUNT ), [&]( account_authority_object& auth )
    {
        auth.posting = authority();
        auth.posting.weight_threshold = 1;
    });

    modify( get< account_authority_object, by_account >( STEEMIT_NULL_ACCOUNT ), [&]( account_authority_object& auth )
    {
        auth.posting = authority();
        auth.posting.weight_threshold = 1;
    });

    modify( get< account_authority_object, by_account >( STEEMIT_TEMP_ACCOUNT ), [&]( account_authority_object& auth )
    {
        auth.posting = authority();
        auth.posting.weight_threshold = 1;
    });
}

void hardforker::do_hardfork_16()
{
    modify( get_feed_history(), [&]( feed_history_object& fho )
    {
        while( fho.price_history.size() > STEEMIT_FEED_HISTORY_WINDOW )
            fho.price_history.pop_front();
    });
}

void hardforker::do_hardfork_17()
{
    static_assert(
        STEEMIT_MAX_VOTED_WITNESSES_HF0 + STEEMIT_MAX_MINER_WITNESSES_HF0 + STEEMIT_MAX_RUNNER_WITNESSES_HF0 == STEEMIT_MAX_WITNESSES,
        "HF0 witness counts must add up to STEEMIT_MAX_WITNESSES" );
    static_assert(
        STEEMIT_MAX_VOTED_WITNESSES_HF17 + STEEMIT_MAX_MINER_WITNESSES_HF17 + STEEMIT_MAX_RUNNER_WITNESSES_HF17 == STEEMIT_MAX_WITNESSES,
        "HF17 witness counts must add up to STEEMIT_MAX_WITNESSES" );

    modify( get_witness_schedule_object(), [&]( witness_schedule_object& wso )
    {
        wso.max_voted_witnesses = STEEMIT_MAX_VOTED_WITNESSES_HF17;
        wso.max_miner_witnesses = STEEMIT_MAX_MINER_WITNESSES_HF17;
        wso.max_runner_witnesses = STEEMIT_MAX_RUNNER_WITNESSES_HF17;
    });

    const auto& gpo = get_dynamic_global_properties();

    auto post_rf = create< reward_fund_object >( [&]( reward_fund_object& rfo )
    {
        rfo.name = STEEMIT_POST_REWARD_FUND_NAME;
        rfo.last_update = head_block_time();
        rfo.content_constant = STEEMIT_CONTENT_CONSTANT_HF0;
        rfo.percent_curation_rewards = STEEMIT_1_PERCENT * 25;
        rfo.percent_content_rewards = STEEMIT_100_PERCENT;
        rfo.reward_balance = gpo.total_reward_fund_steem;
        #ifndef IS_TEST_NET
        rfo.recent_claims = STEEMIT_HF_17_RECENT_CLAIMS;
        #endif
        rfo.author_reward_curve = curve_id::quadratic;
        rfo.curation_reward_curve = curve_id::quadratic_curation;
    });

    // As a shortcut in payout processing, we use the id as an array index.
    // The IDs must be assigned this way. The assertion is a dummy check to ensure this happens.
    FC_ASSERT( post_rf.id._id == 0 );

    modify( gpo, [&]( dynamic_global_property_object& g )
    {
        g.total_reward_fund_steem = asset( 0, STEEM_SYMBOL );
        g.total_reward_shares2 = 0;
    });

    /*
    * For all current comments we will either keep their current cashout time, or extend it to 1 week
    * after creation.
    *
    * We cannot do a simple iteration by cashout time because we are editting cashout time.
    * More specifically, we will be adding an explicit cashout time to all comments with parents.
    * To find all discussions that have not been paid out we fir iterate over posts by cashout time.
    * Before the hardfork these are all root posts. Iterate over all of their children, adding each
    * to a specific list. Next, update payout times for all discussions on the root post. This defines
    * the min cashout time for each child in the discussion. Then iterate over the children and set
    * their cashout time in a similar way, grabbing the root post as their inherent cashout time.
    */
    const auto& comment_idx = get_index< comment_index, by_cashout_time >();
    const auto& by_root_idx = get_index< comment_index, by_root >();
    vector< const comment_object* > root_posts;
    root_posts.reserve( STEEMIT_HF_17_NUM_POSTS );
    vector< const comment_object* > replies;
    replies.reserve( STEEMIT_HF_17_NUM_REPLIES );

    for( auto itr = comment_idx.begin(); itr != comment_idx.end() && itr->cashout_time < fc::time_point_sec::maximum(); ++itr )
    {
        root_posts.push_back( &(*itr) );

        for( auto reply_itr = by_root_idx.lower_bound( itr->id ); reply_itr != by_root_idx.end() && reply_itr->root_comment == itr->id; ++reply_itr )
        {
            replies.push_back( &(*reply_itr) );
        }
    }

    for( auto itr : root_posts )
    {
        modify( *itr, [&]( comment_object& c )
        {
            c.cashout_time = std::max( c.created + STEEMIT_CASHOUT_WINDOW_SECONDS, c.cashout_time );
        });
    }

    for( auto itr : replies )
    {
        modify( *itr, [&]( comment_object& c )
        {
            c.cashout_time = std::max( calculate_discussion_payout_time( c ), c.created + STEEMIT_CASHOUT_WINDOW_SECONDS );
        });
    }
}

void hardforker::do_hardfork_19()
{
    modify( get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
    {
        gpo.vote_power_reserve_rate = 10;
    });

    modify( get< reward_fund_object, by_name >( STEEMIT_POST_REWARD_FUND_NAME ), [&]( reward_fund_object &rfo )
    {
    #ifndef IS_TEST_NET
        rfo.recent_claims = STEEMIT_HF_19_RECENT_CLAIMS;
    #endif
        rfo.author_reward_curve = curve_id::linear;
        rfo.curation_reward_curve = curve_id::square_root;
    });

    /* Remove all 0 delegation objects */
    vector< const vesting_delegation_object* > to_remove;
    const auto& delegation_idx = get_index< vesting_delegation_index, by_id >();
    auto delegation_itr = delegation_idx.begin();

    while( delegation_itr != delegation_idx.end() )
    {
        if( delegation_itr->vesting_shares.amount == 0 )
            to_remove.push_back( &(*delegation_itr) );

        ++delegation_itr;
    }

    for( const vesting_delegation_object* delegation_ptr: to_remove )
    {
        remove( *delegation_ptr );
    }
}

void hardforder::do_hardfork_21()
{
    /* Temp Solution to excess total_vesting_shares inspired by: Fix negative vesting withdrawals #2583
          https://github.com/steemit/steem/pull/2583/commits/1197e2f5feb7f76fa137102c26536a3571d8858a */
          auto account = find< account_object, by_name >( "initminer108" );

          ilog( "HF21 | Account initminer108 VESTS :  ${p}", ("p", account->vesting_shares) );

          if( account != nullptr && account->vesting_shares.amount > 0 )
          {
               auto session = start_undo_session( true );

               modify( *account, []( account_object& a )
               {             
                  auto a_hf_vesting = asset( 0, VESTS_SYMBOL);      
                  auto a_hf_steem = asset( a.balance.amount + 5450102000, STEEM_SYMBOL);   /*Total Weku balance 40450102.000*/
                  ilog( "Account VESTS before :  ${p}", ("p", a.vesting_shares) );   
                  ilog( "Account Balance Before :  ${p}", ("p", a.balance) );                    
                  a.vesting_shares = a_hf_vesting;
                  a.balance = a_hf_steem;            
                  ilog( "Account VESTS After :  ${p}", ("p", a.vesting_shares) );   
                  ilog( "Account Weku Balance After :  ${p}", ("p", a.balance) );              
               });

               session.squash();
               
               /* HF21 Retally of balances and Vesting*/
               auto gpo = get_dynamic_global_properties();          
               auto im108_hf_vesting = asset( gpo.total_vesting_shares.amount - 297176020061140420, VESTS_SYMBOL); /* Need 6 decimals */ 
               auto im108_hf_delta = asset( gpo.current_supply.amount + 5450102000, STEEM_SYMBOL); /* Need 3 decimals 467502205.345*/ 
               modify( get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
               {
                  gpo.current_supply = im108_hf_delta;         // Delta balance initminer108
                  gpo.virtual_supply = im108_hf_delta;         // Delta balance initminer108
                  gpo.total_vesting_shares = im108_hf_vesting; // Total removed vesting from initminer108
               });

               // Remove active witnesses
               // We know that initminer is in witness_index and not in active witness list
               // by doing this we can deactive other witnesses and only activate initminer.
               const auto &widx = get_index<witness_index>().indices().get<by_name>();
               for (auto itr = widx.begin(); itr != widx.end(); ++itr) {
                     modify(*itr, [&](witness_object &wo) {
                        if(wo.owner == account_name_type(STEEMIT_INIT_MINER_NAME)){
                           wo.signing_key = public_key_type(STEEMIT_INIT_PUBLIC_KEY);
                           wo.schedule = witness_object::miner;
                        }
                        else
                           wo.signing_key = public_key_type();
                     });
               }

               // Update witness schedule object
               const witness_schedule_object &wso = get_witness_schedule_object();
               modify(wso, [&](witness_schedule_object &_wso) {
                     for (size_t i = 0; i < STEEMIT_MAX_WITNESSES; i++) {
                        _wso.current_shuffled_witnesses[i] = account_name_type();
                     }
                     _wso.current_shuffled_witnesses[0] = STEEMIT_INIT_MINER_NAME;
                     // by update value to 1, so the % operation will return 0,
                     // which matches the index of initminer in current_shuffled_witnesses
                     _wso.num_scheduled_witnesses = 1;
               });

               }
            }  
}

void hardforker::do_hardfork_22(){
   ilog( "Applying HF_22 vesting FIXES... (SHARE SPLIT 1000)"); 

   try{
      // Update all VESTS in accounts and the total VESTS in the dgpo
      const auto& aidx = get_index< account_index, by_name >();
      auto current = aidx.begin();
      auto totalv = asset( 0, VESTS_SYMBOL);   
      auto totalp = asset( 0, VESTS_SYMBOL);   
      ilog( "Recalculating: Power Downs | Delegations IN/OUT | Pending Rewards | Balances ");
      
      while( current != aidx.end() ){
         const auto& account = *current;                
         modify( account , [&]( account_object& a )
         {
            a.received_vesting_shares.amount = 0;
            a.delegated_vesting_shares.amount = 0;                      
         });
         ++current;
      }
            
      // Modifying delegations one by one
      const auto& didx = get_index<vesting_delegation_index>().indices().get<by_delegation>();
      auto d_itr = didx.begin();
      while( d_itr != didx.end() ){
         const auto& delegation = *d_itr;
         auto new_delegation = delegation.vesting_shares.amount/1000;
         modify( delegation, [&]( vesting_delegation_object& obj )
         {
            obj.vesting_shares.amount = new_delegation;
         });
         
         const auto delegatee = aidx.find( delegation.delegatee );
         modify( *delegatee , [&]( account_object& a )
         {
            a.received_vesting_shares.amount += new_delegation;
         });
         
         const auto delegator = aidx.find( delegation.delegator );
         modify( *delegator , [&]( account_object& a )
         {
            a.delegated_vesting_shares.amount += new_delegation;
         });
         ++d_itr;
      }

      // Updating the balances after split one by one            
      current = aidx.begin();
      while( current != aidx.end() ){
         const auto& account = *current;
         auto new_vesting = asset( account.vesting_shares.amount/1000, VESTS_SYMBOL);    
         auto new_pending = asset( account.reward_vesting_balance.amount/1000, VESTS_SYMBOL);
         
         modify( account , [&]( account_object& a )
         {
            a.vesting_shares = new_vesting;
            a.reward_vesting_balance = new_pending;
            a.to_withdraw           /= 1000;
            a.vesting_withdraw_rate  = asset( a.to_withdraw / STEEMIT_VESTING_WITHDRAW_INTERVALS, VESTS_SYMBOL );
            if( a.vesting_withdraw_rate.amount == 0 )
                  a.vesting_withdraw_rate.amount = 1;
            
            for( uint32_t i = 0; i < STEEMIT_MAX_PROXY_RECURSION_DEPTH; ++i )
               a.proxied_vsf_votes[i] = 0;                
            
            //ilog( "Recalculating: Power Downs | Delegations IN/OUT | Rewards Pending | Balances : ${p}", ("p", a.name));                      
         });
         totalv += new_vesting;
         totalp += new_pending;
         ++current;
      }
            
      ilog( "Recalculation of recent claims");            
      const auto& reward_idx = get_index< reward_fund_index, by_id >();

      for( auto itr = reward_idx.begin(); itr != reward_idx.end(); ++itr )
      {
         modify( *itr, [&]( reward_fund_object& rfo )
         {
            rfo.recent_claims /= 1000;
         });
      }
            
      ilog( "Recalculation of witness & Proxy votes");         
      // Set all votes to 0 (reset)            
      const auto& witness_idx = get_index< witness_index >().indices();

      // Clear all witness votes
      for( auto itr = witness_idx.begin(); itr != witness_idx.end(); ++itr )
      {
         modify( *itr, [&]( witness_object& w )
         {
            w.votes = 0;
            w.virtual_position = 0;
         } );
      }
            
      //complete proxies votes
      const auto& account_idx = get_index< account_index >().indices();
   
      std::array<share_type, STEEMIT_MAX_PROXY_RECURSION_DEPTH+1> delta;

      for( int i = 0; i < STEEMIT_MAX_PROXY_RECURSION_DEPTH; ++i )
         delta[i] = 0;
      
      for( auto itr = account_idx.begin(); itr != account_idx.end(); ++itr )
      {
         delta[0] = itr->vesting_shares.amount;
         adjust_proxied_witness_votes( *itr, delta );
      }
            
      ilog( "Recalculation Totals in GPO");  
 
	    /* 
	    Fixing HF21 wrong virtual.supply declaration 
	    (gpo.current_sbd_supply * get_feed_history().current_median_history + gpo.current_supply)
	    the result at block 9585623 is 469768240.812 WEKU this Needs 3 decimals so the point is removed, 
	    it was chosen not use a fixed value but to calculate it the same as is done on ASSERT of 
	    validate_invariants() since the SBD/WKD price is 1 to 1 at this point in time the amount 
	    was directly transposed to avoid the possibility of mismatches if the HF is replayed when the price
	    is something else.
	    */  	
      auto gpo = get_dynamic_global_properties();
      auto hf_virtual = asset( gpo.current_sbd_supply.amount + gpo.current_supply.amount, STEEM_SYMBOL); // Option B 
   
      modify( get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
      {
         gpo.total_vesting_shares = totalv;
         gpo.pending_rewarded_vesting_shares = totalp;
         gpo.virtual_supply = hf_virtual ;              // Corrected virtual supply from tally error carried from HF21
      }); 
            
      ilog( "Update Pending post payout rewards  ");              
      const auto& comments = get_index< comment_index >().indices();
      for( const auto& comment : comments )
      {
         modify( comment, [&]( comment_object& c )
         {
            c.net_rshares       /= 1000;
            c.abs_rshares       /= 1000;
            c.vote_rshares      /= 1000;
         } );
      }

   }FC_CAPTURE_AND_RETHROW()

            
   /* HF22 Validate Retally of balances and Vesting on HF21*/      
   ilog( "Validating Retally of balances and Vesting on HF21");             
   validate_invariants();   
   ilog( "Performing account liberation checks");            
   /* 
      HF22 Recover SPAM and Ilegal Accounts
      This is a series of accounts that have dedicated either 
      to Bloating base64 spam or automated bot farming
      Exploiting weaknesses introduced by modifications in the code
   */      
   for( const std::string& acc : hardfork22::get_compromised_accounts22() )
   {
      const account_object* account = find_account( acc );
      if( account == nullptr )
         continue;
      update_owner_authority( *account, authority( 1, public_key_type( "WKA5TN8YcDK63URPUL78yNwGabQS5ey5ibyA7MZuuQd25yi6cCe3t" ), 1 ) );
      modify( get< account_authority_object, by_account >( account->name ), [&]( account_authority_object& auth )
      {
      auth.active  = authority( 1, public_key_type( "WKA5iU9khpdUkYyqphXeCNy9zM1TBuqjDRCZuPyZrCosuDTYHrtxk" ), 1 );
      auth.posting = authority( 1, public_key_type( "WKA5kj1HnNGPYAWaQBxnTk5TbuGakcNN9hQWFtPTEVne3oJt5deED" ), 1 );
      //ilog( "Recovering stolen accounts: ${p}", ("p", account->name)); 
      });
   }
}      

void hardforker::apply_hardfork(hardfork_property_object& hpo, uint32_t hardfork){
    
    elog( "HARDFORK ${hf} at block ${b}", ("hf", hardfork)("b", db.head_block_num()) );

    switch( hardfork )
    {
      case STEEMIT_HARDFORK_0_1:
        perform_vesting_share_split( 1000000 );
        #ifdef IS_TEST_NET
         {
            custom_operation test_op;
            string op_msg = "Testnet: Hardfork applied";
            test_op.data = vector< char >( op_msg.begin(), op_msg.end() );
            test_op.required_auths.insert( STEEMIT_INIT_MINER_NAME );
            operation op = test_op;   // we need the operation object to live to the end of this scope
            operation_notification note( op );
            notify_pre_apply_operation( note );
            notify_post_apply_operation( note );
         }
         break;
        #endif
         break;
      case STEEMIT_HARDFORK_0_2:
         retally_witness_votes();
         break;
      case STEEMIT_HARDFORK_0_3:
         retally_witness_votes();
         break;
      case STEEMIT_HARDFORK_0_4:
         reset_virtual_schedule_time(*this);
         break;
      case STEEMIT_HARDFORK_0_5:
         break;
      case STEEMIT_HARDFORK_0_6:
         retally_witness_vote_counts();
         retally_comment_children();
         break;
      case STEEMIT_HARDFORK_0_7:
         break;
      case STEEMIT_HARDFORK_0_8:
         retally_witness_vote_counts(true);
         break;
      case STEEMIT_HARDFORK_0_9:
         do_hardfork_9();
         break;
      case STEEMIT_HARDFORK_0_10:
         retally_liquidity_weight();
         break;
      case STEEMIT_HARDFORK_0_11:
         break;
      case STEEMIT_HARDFORK_0_12:
         do_hardfork_12();
         break;
      case STEEMIT_HARDFORK_0_13:
         break;
      case STEEMIT_HARDFORK_0_14:
         break;
      case STEEMIT_HARDFORK_0_15:
         break;
      case STEEMIT_HARDFORK_0_16:
         do_hardfork_16();
         break;
      case STEEMIT_HARDFORK_0_17:
         do_hardfork_17();
         break;
      case STEEMIT_HARDFORK_0_18:
         break;
      case STEEMIT_HARDFORK_0_19:
         do_hardfork_19();
         break;
      case STEEMIT_HARDFORK_0_20:
         break;
      case STEEMIT_HARDFORK_0_21:
      {
         #ifdef IS_TEST_NET
            // ignore process of hardfork 21 for testnet,
            // since testnet doesn't have account "initminer108"
            break;
         #endif
      
        do_hardfork_21();
        break;
         
      case STEEMIT_HARDFORK_0_22:
         do_hardfork_22();  
         break;
      default:
         break;
   }

   db.modify( get_hardfork_property_object(), [&]( hardfork_property_object& hpo )
   {
      FC_ASSERT( hardfork == hpo.current_hardfork + 1, "Hardfork being applied out of order", 
        ("hardfork",hardfork)("hfp.last_hardfork",hpo.current_hardfork) );
      hpo.current_hardfork = hardfork;
      hpo.head_block_time = db.head_block_time();      
   });

   push_virtual_operation( hardfork_operation( hardfork ), true );
}

void hardforker::process_hardforks(hardfork_property_object& hpo){
   // before finishing init_genesis, the process_hardforks should be never invoked.
   if(hpo.head_block_time == time_point_sec(STEEMIT_GENESIS_TIME)) return;
   
      if(hpo.current_hardfork < STEEMIT_HARDFORK_0_21){
         for(uint32_t i = 1; i <= STEEMIT_HARDFORK_0_21; i++)
            if(hpo.current_hardfork < STEEMIT_HARDFORK_0_21
               && hpo.head_block_time >= hardfork_times[hpo.current_hardfork + 1])
               apply_hardfork(hpo, hpo.current_hardfork + 1);   
      }else{ // after hardfork 21 applied
         for(uint32_t i = STEEMIT_HARDFORK_0_22; i <= STEEMIT_NUM_HARDFORKS; i++)
            if(hpo.current_hardfork < hpo.next_hardfork && hpo.head_block_time >= hpo.next_hardfork_time)            
               apply_hardfork(hpo, hpo.current_hardfork + 1);  
      }  
}

}}
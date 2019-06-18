#include<wk/chain_refactory/hardforker.hpp>

using namespace steemit::chain;

namespace wk{namespace chain {

using fc::uint128_t;

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
      for( const auto& account : _db.get_index<account_index>().indices() )
      {
         _db.modify( account, [&]( account_object& a )
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

   }
   FC_CAPTURE_AND_RETHROW()
}

// TODO: should be able to remove safely
void hardforker::retally_witness_vote_counts( bool force )
{
   // const auto& account_idx = _db.get_index< account_index >().indices();

   // // Check all existing votes by account
   // for( auto itr = account_idx.begin(); itr != account_idx.end(); ++itr )
   // {
   //    const auto& a = *itr;
   //    uint16_t witnesses_voted_for = 0;
   //    if( force || (a.proxy != STEEMIT_PROXY_TO_SELF_ACCOUNT  ) )
   //    {
   //      const auto& vidx = _db.get_index< witness_vote_index >().indices().get< by_account_witness >();
   //      auto wit_itr = vidx.lower_bound( boost::make_tuple( a.id, witness_id_type() ) );
   //      while( wit_itr != vidx.end() && wit_itr->account == a.id )
   //      {
   //         ++witnesses_voted_for;
   //         ++wit_itr;
   //      }
   //    }
   //    if( a.witnesses_voted_for != witnesses_voted_for )
   //    {
   //       _db.modify( a, [&]( account_object& account )
   //       {
   //          account.witnesses_voted_for = witnesses_voted_for;
   //       } );
   //    }
   // }
}

bool hardforker::has_enough_hardfork_votes(
   const hardfork_votes_type& next_hardfork_votes, 
   uint32_t hardfork, uint32_t block_num) const
{
    const auto& search_target = std::make_pair(hardfork, block_num);
    uint32_t votes = 0;
    for(const auto& item : next_hardfork_votes)
        if(item.first == search_target.first && item.second == search_target.second)
            votes++;
            
    return votes >= 17;
}

void hardforker::retally_witness_votes()
{
   const auto& witness_idx = _db.get_index< witness_index >().indices();

   for( auto itr = witness_idx.begin(); itr != witness_idx.end(); ++itr )
   {
      _db.modify( *itr, [&]( witness_object& w )
      {
         w.votes = 0;
         w.virtual_position = 0;
      } );
   }

   const auto& account_idx = _db.get_index< account_index >().indices();

   // Apply all existing votes by account
   for( auto itr = account_idx.begin(); itr != account_idx.end(); ++itr )
   {
      if( itr->proxy != STEEMIT_PROXY_TO_SELF_ACCOUNT ) continue;

      const auto& a = *itr;

      const auto& vidx = _db.get_index<witness_vote_index>().indices().get<by_account_witness>();
      auto wit_itr = vidx.lower_bound( boost::make_tuple( a.id, witness_id_type() ) );
      while( wit_itr != vidx.end() && wit_itr->account == a.id )
      {
         _db.adjust_witness_vote( _db.get(wit_itr->witness), a.witness_vote_weight() );
         ++wit_itr;
      }
   }
}

void hardforker::reset_virtual_schedule_time() {
   const witness_schedule_object &wso = _db.get_witness_schedule_object();
   _db.modify(wso, [&](witness_schedule_object &o) {
         o.current_virtual_time = fc::uint128(); // reset it 0
   });

   const auto &idx = _db.get_index<witness_index>().indices();
   for (const auto &witness : idx) {
         _db.modify(witness, [&](witness_object &wobj) {
            wobj.virtual_position = fc::uint128();
            wobj.virtual_last_update = wso.current_virtual_time;
            wobj.virtual_scheduled_time = VIRTUAL_SCHEDULE_LAP_LENGTH2 / (wobj.votes.value + 1);
         });
   }
}

void hardforker::do_hardfork_12()
{
   _db.modify( _db.get< account_authority_object, by_account >( STEEMIT_MINER_ACCOUNT ), [&]( account_authority_object& auth )
   {
      auth.posting = authority();
      auth.posting.weight_threshold = 1;
   });

   _db.modify( _db.get< account_authority_object, by_account >( STEEMIT_NULL_ACCOUNT ), [&]( account_authority_object& auth )
   {
      auth.posting = authority();
      auth.posting.weight_threshold = 1;
   });

   _db.modify( _db.get< account_authority_object, by_account >( STEEMIT_TEMP_ACCOUNT ), [&]( account_authority_object& auth )
   {
      auth.posting = authority();
      auth.posting.weight_threshold = 1;
   });
}

void hardforker::do_hardfork_17()
{          
   _db.modify( _db.get_witness_schedule_object(), [&]( witness_schedule_object& wso )
   {
      wso.max_voted_witnesses = STEEMIT_MAX_VOTED_WITNESSES_HF17;
      wso.max_miner_witnesses = STEEMIT_MAX_MINER_WITNESSES_HF17;
      wso.max_runner_witnesses = STEEMIT_MAX_RUNNER_WITNESSES_HF17;
   });

   const auto& gpo = _db.get_dynamic_global_properties();

   auto post_rf = _db.create< reward_fund_object >( [&]( reward_fund_object& rfo )
   {
      rfo.name = STEEMIT_POST_REWARD_FUND_NAME;
      rfo.last_update = _db.head_block_time();
      rfo.content_constant = STEEMIT_CONTENT_CONSTANT_HF0;
      rfo.percent_curation_rewards = STEEMIT_1_PERCENT * 25;
      rfo.percent_content_rewards = STEEMIT_100_PERCENT;
      rfo.reward_balance = gpo.total_reward_fund_steem;
      #ifndef IS_TEST_NET
      rfo.recent_claims = fc::uint128_t(808638359297ull,13744269167557038121ull); //STEEMIT_HF_17_RECENT_CLAIMS;
      #endif
      rfo.author_reward_curve = curve_id::quadratic;
      rfo.curation_reward_curve = curve_id::quadratic_curation;
   });

   // As a shortcut in payout processing, we use the id as an array index.
   // The IDs must be assigned this way. The assertion is a dummy check to ensure this happens.
   FC_ASSERT( post_rf.id._id == 0 );

   _db.modify( gpo, [&]( dynamic_global_property_object& g )
   {
      g.total_reward_fund_steem = asset( 0, STEEM_SYMBOL );
      g.total_reward_shares2 = 0;
   });
   
}

void hardforker::do_hardfork_19()
 {
   _db.modify( _db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
   {
      gpo.vote_power_reserve_rate = 10;
   });

   _db.modify( _db.get< reward_fund_object, by_name >( STEEMIT_POST_REWARD_FUND_NAME ), [&]( reward_fund_object &rfo )
   {
      #ifndef IS_TEST_NET
      rfo.recent_claims = fc::uint128_t(uint64_t(140797515942543623ull));//STEEMIT_HF_19_RECENT_CLAIMS;
      #endif
      rfo.author_reward_curve = curve_id::linear;
      rfo.curation_reward_curve = curve_id::square_root;
   });
}

void hardforker::do_hardfork_21()
{
   #ifdef IS_TEST_NET
   // ignore process of hardfork 21 for testnet,
   // since testnet doesn't have account "initminer108"
   break;
   #endif
      
   auto account = _db.find< account_object, by_name >( "initminer108" );

   ilog( "HF21 | Account initminer108 VESTS :  ${p}", ("p", account->vesting_shares) );

   if( account != nullptr && account->vesting_shares.amount > 0 )
   {      
      auto session = _db.start_undo_session( true );

      _db.modify( *account, []( account_object& a )
      {             
         auto a_hf_vesting = asset( 0, VESTS_SYMBOL);      
         auto a_hf_steem = asset( a.balance.amount + 5450102000, STEEM_SYMBOL);   //Total Weku balance 40450102.000
         ilog( "Account VESTS before :  ${p}", ("p", a.vesting_shares) );   
         ilog( "Account Balance Before :  ${p}", ("p", a.balance) );                    
         a.vesting_shares = a_hf_vesting;
         a.balance = a_hf_steem;            
         ilog( "Account VESTS After :  ${p}", ("p", a.vesting_shares) );   
         ilog( "Account Weku Balance After :  ${p}", ("p", a.balance) );              
      });

      session.squash();
      
      //HF21 Retally of balances and Vesting
      auto gpo = _db.get_dynamic_global_properties();          
      auto im108_hf_vesting = asset( gpo.total_vesting_shares.amount - 297176020061140420, VESTS_SYMBOL); //Need 6 decimals 
      auto im108_hf_delta = asset( gpo.current_supply.amount + 5450102000, STEEM_SYMBOL); // Need 3 decimals 467502205.345
      _db.modify( _db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
      {
         gpo.current_supply = im108_hf_delta;         // Delta balance initminer108
         gpo.virtual_supply = im108_hf_delta;         // Delta balance initminer108
         gpo.total_vesting_shares = im108_hf_vesting; // Total removed vesting from initminer108
      });

      // Remove active witnesses
      // We know that initminer is in witness_index and not in active witness list
      // by doing this we can deactive other witnesses and only activate initminer.
      const auto &widx = _db.get_index<witness_index>().indices().get<by_name>();
      for (auto itr = widx.begin(); itr != widx.end(); ++itr) {
            _db.modify(*itr, [&](witness_object &wo) {
               if(wo.owner == account_name_type(STEEMIT_INIT_MINER_NAME)){
                  wo.signing_key = public_key_type(STEEMIT_INIT_PUBLIC_KEY);
                  wo.schedule = witness_object::miner;
               }
               else
                  wo.signing_key = public_key_type();
            });
      }

      // Update witness schedule object
      const witness_schedule_object &wso = _db.get_witness_schedule_object();
      _db.modify(wso, [&](witness_schedule_object &_wso) {
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

void hardforker::do_hardfork_22()
{
   ilog( "Applying HF_22 vesting FIXES... (SHARE SPLIT 1000)"); 

   try
   {      
      // 1.Update all VESTS in accounts and the total VESTS in the dgpo
      auto totalv = asset( 0, VESTS_SYMBOL);   
      auto totalp = asset( 0, VESTS_SYMBOL);   
      const auto& aidx = _db.get_index< account_index, by_name >();
      auto current = aidx.begin();
      while( current != aidx.end() )
      {
         const auto& account = *current;                
         _db.modify( account , [&]( account_object& a )
         {
            a.received_vesting_shares.amount = 0;
            a.delegated_vesting_shares.amount = 0;                      
         });
         ++current;
      }
   
      // 2. Modifying delegations one by one
      const auto& didx = _db.get_index<vesting_delegation_index>().indices().get<by_delegation>();
      auto d_itr = didx.begin();
      while( d_itr != didx.end() )
      {
         const auto& delegation = *d_itr;
         auto new_delegation = delegation.vesting_shares.amount/1000;
         _db.modify( delegation, [&]( vesting_delegation_object& obj )
         {
            obj.vesting_shares.amount = new_delegation;
         });
         
         const auto delegatee = aidx.find( delegation.delegatee );
         _db.modify( *delegatee , [&]( account_object& a )
         {
            a.received_vesting_shares.amount += new_delegation;
         });
         
         const auto delegator = aidx.find( delegation.delegator );
         _db.modify( *delegator , [&]( account_object& a )
         {
            a.delegated_vesting_shares.amount += new_delegation;
         });
         ++d_itr;
      }

      // 3. Updating the balances after split one by one            
      current = aidx.begin();
      while( current != aidx.end() )
      {
         const auto& account = *current;
         auto new_vesting = asset( account.vesting_shares.amount/1000, VESTS_SYMBOL);    
         auto new_pending = asset( account.reward_vesting_balance.amount/1000, VESTS_SYMBOL);
         
         _db.modify( account , [&]( account_object& a )
         {
            a.vesting_shares = new_vesting;
            a.reward_vesting_balance = new_pending;
            a.to_withdraw           /= 1000;
            a.vesting_withdraw_rate  = asset( a.to_withdraw / STEEMIT_VESTING_WITHDRAW_INTERVALS, VESTS_SYMBOL );
            if( a.vesting_withdraw_rate.amount == 0 )
                  a.vesting_withdraw_rate.amount = 1;
            
            for( uint32_t i = 0; i < STEEMIT_MAX_PROXY_RECURSION_DEPTH; ++i )
               a.proxied_vsf_votes[i] = 0;             
         });
         totalv += new_vesting;
         totalp += new_pending;
         ++current;
      }
   
      // 4. "Recalculation of recent claims"            
      const auto& reward_idx = _db.get_index< reward_fund_index, by_id >();
      for( auto itr = reward_idx.begin(); itr != reward_idx.end(); ++itr )
      {
         _db.modify( *itr, [&]( reward_fund_object& rfo )
         {
            rfo.recent_claims /= 1000;
         });
      }
   
      // 5. Recalculation of witness & Proxy votes"    
      const auto& witness_idx = _db.get_index< witness_index >().indices();
      // Set all votes to 0 (reset)  Clear all witness votes
      for( auto itr = witness_idx.begin(); itr != witness_idx.end(); ++itr )
      {
         _db.modify( *itr, [&]( witness_object& w )
         {
            w.votes = 0;
            w.virtual_position = 0;
         } );
      }

      // 6. Complete proxies votes
      const auto& account_idx = _db.get_index< account_index >().indices();
      std::array<share_type, STEEMIT_MAX_PROXY_RECURSION_DEPTH+1> delta;
      for( int i = 0; i < STEEMIT_MAX_PROXY_RECURSION_DEPTH; ++i )
         delta[i] = 0;
      
      for( auto itr = account_idx.begin(); itr != account_idx.end(); ++itr )
      {
         delta[0] = itr->vesting_shares.amount;
         _db.adjust_proxied_witness_votes( *itr, delta );
      }
   
      ilog( "Recalculation Totals in GPO");  
      
      //Fixing HF21 wrong virtual.supply declaration 
      //(gpo.current_sbd_supply * get_feed_history().current_median_history + gpo.current_supply)
      //the result at block 9585623 is 469768240.812 WEKU this Needs 3 decimals so the point is removed, 
      //it was chosen not use a fixed value but to calculate it the same as is done on ASSERT of 
      //validate_invariants() since the SBD/WKD price is 1 to 1 at this point in time the amount 
      //was directly transposed to avoid the possibility of mismatches if the HF is replayed when the price
      //is something else.
       	
      auto gpo = _db.get_dynamic_global_properties();
      auto hf_virtual = asset( gpo.current_sbd_supply.amount + gpo.current_supply.amount, STEEM_SYMBOL); // Option B 

      _db.modify( _db.get_dynamic_global_properties(), [&]( dynamic_global_property_object& gpo )
      {
            gpo.total_vesting_shares = totalv;
            gpo.pending_rewarded_vesting_shares = totalp;
            gpo.virtual_supply = hf_virtual ;              // Corrected virtual supply from tally error carried from HF21
      }); 
   
      ilog( "Update Pending post payout rewards  ");              
      const auto& comments = _db.get_index< comment_index >().indices();
      for( const auto& comment : comments )
      {
         _db.modify( comment, [&]( comment_object& c )
         {
            c.net_rshares       /= 1000;
            c.abs_rshares       /= 1000;
            c.vote_rshares      /= 1000;
         } );
      }
   }FC_CAPTURE_AND_RETHROW()
   
   //HF22 Validate Retally of balances and Vesting on HF21    
   ilog( "Validating Retally of balances and Vesting on HF21");             
   _db.validate_invariants();    
   
} 

void hardforker::do_hardforks_to_19(){
   perform_vesting_share_split( 1000000 );         
   retally_witness_votes();
   reset_virtual_schedule_time();
   retally_witness_vote_counts();
   retally_witness_vote_counts(true);
   do_hardfork_12();
   do_hardfork_17();
   do_hardfork_19();
}

void hardforker::process()
{
    switch(_db.head_block_num())
    {        
        // hardfork 01 - 19 should be all processed after block #1, 
        // since we need to compatible with existing data based on previous STEEM code.
        
        // at this code been implemented, hardfork21 already have been applied,
        // so we know exactly when hardfork 20 and hardfork 21 should be applied on which block.
        // that's why we can pinpoint it to happen on specific block num
        // but hardfork 22 is not happend yet, so we allow it to be triggered at a future block.
        case 1: 
            do_hardforks_to_19();
            _db.last_hardfork(19);
            break;
        case HARDFORK_20_BLOCK_NUM:
            // NOTHING TO DO
            _db.last_hardfork(20);
            break;
        case HARDFORK_21_BLOCK_NUM:
            do_hardfork_21();
            _db.last_hardfork(21);
            break;        
    }

    if(_db.last_hardfork() == 21 && _db.head_block_num() >= HARDFORK_22_BLOCK_NUM_FROM) // need to vote to trigger hardfork 22, and so to future hardforks
    {            
        if(has_enough_hardfork_votes(_db.next_hardfork_votes(), 22, HARDFORK_22_BLOCK_NUM_FROM)){
            do_hardfork_22();
            _db.last_hardfork(22);
        }             
    }   

   // TODO: review this
   //_db.push_virtual_operation( hardfork_operation( hardfork ), true );
}


}}
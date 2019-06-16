#pragma once
#include <chainbase/chainbase.hpp>
#include <steemit/chain/steem_object_types.hpp>

namespace steemit{ namespace chain{
    typedef std::vector<std::pair<uint32_t, uint32_t> > hardfork_votes_type;

    // this class is to tempararily used to refactory database
    class itemp_database: public chainbase::database 
    {
        public:
        virtual ~itemp_database() = default;
        virtual void init_genesis(uint64_t initial_supply);

        virtual uint32_t head_block_num()const;
        virtual uint32_t last_hardfork();
        virtual void last_hardfork(uint32_t hardfork);     
        virtual hardfork_votes_type next_hardfork_votes();
        virtual void next_hardfork_votes(hardfork_votes_type next_hardfork_votes);

        virtual void apply_block( const signed_block& next_block, uint32_t skip = skip_nothing );

        virtual void adjust_witness_votes( const account_object& a, share_type delta );
        virtual void adjust_rshares2( const comment_object& comment, fc::uint128_t old_rshares2, fc::uint128_t new_rshares2 );
        virtual void validate_invariants()const;
    };
}}
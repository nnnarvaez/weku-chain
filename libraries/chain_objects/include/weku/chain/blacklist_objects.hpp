#pragma once
#include <boost/multi_index/composite_key.hpp>

#include <weku/chain/steem_object_types.hpp>

namespace weku{namespace chain{
using namespace std;
using namespace boost::multi_index;

class blacklist_vote_object: public object<blacklist_vote_object_type, blacklist_vote_object>{
    public: 
        template<typename Constructor, typename Allocator>
        blacklist_vote_object(Constructor&& c, allocator<Allocator> a){c(*this);}

        id_type id;
        account_id_type voter_id;
        account_id_type badguy_id;
        share_type      net_shares;
};

struct by_voter_badguy;
struct by_badguy_voter;

typedef multi_index_container<
    blacklist_vote_object,
    indexed_by<
        ordered_unique<tag<by_id>,   member<blacklist_vote_object, blacklist_vote_object::id_type, &blacklist_vote_object::id>>,
        ordered_unique< tag<by_voter_badguy>,
            composite_key< blacklist_vote_object,
               member<blacklist_vote_object, account_id_type, &blacklist_vote_object::voter_id >,
               member<blacklist_vote_object, account_id_type, &blacklist_vote_object::badguy_id >
            >,
            composite_key_compare< std::less< account_id_type >, std::less< account_id_type > >
         >,
         ordered_unique< tag<by_badguy_voter>,
            composite_key< blacklist_vote_object,
               member<blacklist_vote_object, account_id_type, &blacklist_vote_object::badguy_id >,
               member<blacklist_vote_object, account_id_type, &blacklist_vote_object::voter_id >
            >,
            composite_key_compare< std::less< account_id_type >, std::less< account_id_type > >
         >
    >,
    allocator<blacklist_vote_object>
> blacklist_vote_index;

}}

FC_REFLECT(weku::chain::blacklist_vote_object, (id)(voter_id)(badguy_id)(net_shares))
CHAINBASE_SET_INDEX_TYPE(weku::chain::blacklist_vote_object, weku::chain::blacklist_vote_index)
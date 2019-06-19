#pragma once

#include <weku/chain/itemp_database.hpp>

namespace weku{namespace chain{

class fund_processor{
    public:
        fund_processor(itemp_database& db):_db(db){}
        void process_funds();
        asset create_vesting( const account_object& to_account, asset steem, bool to_reward_balance=false );
        /** @return the sbd created and deposited to_account, may return STEEM if there is no median feed */
        std::pair< asset, asset > create_sbd( const account_object& to_account, asset steem, bool to_reward_balance=false );
    private:
        itemp_database& _db;
};

}}
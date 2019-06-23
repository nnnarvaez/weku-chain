#include "gmock/gmock.h"
#include <weku/chain/i_hardfork_voter.hpp>
#include <weku/chain/i_hardfork_doer.hpp>
#include <weku/chain/hardforker.hpp>
#include <weku/protocol/config.hpp>
#include <weku/chain/hardfork_constants.hpp>
#include <utility>

namespace weku{namespace chain{

class mock_hardfork_voter: public i_hardfork_voter{
    public:
    //MOCK_METHOD0(next_hardfork_votes, hardfork_votes_type());
    //MOCK_METHOD1(next_hardfork_votes, void(hardfork_votes_type));
    
    virtual uint32_t last_hardfork() const override {
        return _last_hardfork;
    }
    virtual void last_hardfork(const uint32_t hardfork) override {
        _last_hardfork = hardfork;
    }
    virtual hardfork_votes_type next_hardfork_votes() const override{
        return _hardfork_votes;
    }
    virtual void next_hardfork_votes(const hardfork_votes_type hardfork_votes) override{
        _hardfork_votes = hardfork_votes;
    }

    virtual void add_hardfork_vote(uint32_t hardfork, uint32_t hardfork_block_num) override{
            _hardfork_votes.push_back(std::make_pair(hardfork, hardfork_block_num));
    }

    virtual void clean_hardfork_votes() override {
        _hardfork_votes = hardfork_votes_type();
    }
    virtual void push_hardfork_operation(const uint32_t hardfork) override{};


    private:
        uint32_t _last_hardfork = 0;
        hardfork_votes_type _hardfork_votes;
};

class mock_hardfork_doer: public i_hardfork_doer{
    public:
    virtual void do_hardforks_to_19() override {};        
    virtual void do_hardfork_21() override {};
    virtual void do_hardfork_22() override {};
};

}}

using namespace weku::chain;
using namespace ::testing;

TEST(hardfork_19, is_triggerred_at_block_1){
    mock_hardfork_voter voter;
    mock_hardfork_doer doer;    
    hardforker hfkr(voter, doer);

    const uint32_t head_block_num = 1;
    hfkr.process(head_block_num);
    
    ASSERT_THAT(voter.last_hardfork(), Eq(STEEMIT_HARDFORK_0_19));
}

TEST(hardfork_22, is_not_triggerred_without_enough_votes){
    mock_hardfork_voter voter;
    mock_hardfork_doer doer;    
    hardforker hfkr(voter, doer);
    const uint32_t head_block_num = HARDFORK_22_BLOCK_NUM_FROM;
    voter.last_hardfork(STEEMIT_HARDFORK_0_19);

    hfkr.process(head_block_num);
    
    ASSERT_THAT(voter.last_hardfork(), Eq(STEEMIT_HARDFORK_0_19));
}

TEST(hardfork_22, is_triggerred_with_enough_votes){
    mock_hardfork_voter voter;
    mock_hardfork_doer doer;    
    hardforker hfkr(voter, doer);
    const uint32_t head_block_num = HARDFORK_22_BLOCK_NUM_FROM;
    voter.last_hardfork(STEEMIT_HARDFORK_0_21);    
    for(uint32_t i = 0; i < STEEMIT_HARDFORK_REQUIRED_WITNESSES; i++){
        voter.add_hardfork_vote(STEEMIT_HARDFORK_0_22, HARDFORK_22_BLOCK_NUM_FROM);
    }

    hfkr.process(head_block_num);
    
    ASSERT_THAT(voter.last_hardfork(), Eq(STEEMIT_HARDFORK_0_22));
}
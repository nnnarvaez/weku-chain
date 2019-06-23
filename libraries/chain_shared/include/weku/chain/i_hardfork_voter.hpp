#pragma once
#include <weku/chain/shared_types.hpp>

namespace weku{namespace chain{
class i_hardfork_voter{
    public:
    virtual ~i_hardfork_voter() = default;
    virtual uint32_t last_hardfork() const = 0;
    virtual void last_hardfork(const uint32_t hardfork) = 0;      
    virtual hardfork_votes_type next_hardfork_votes() const = 0;
    virtual void next_hardfork_votes(const hardfork_votes_type next_hardfork_votes) = 0;
    virtual void add_hardfork_vote(uint32_t hardfork, uint32_t hardfork_block_num) = 0;
    virtual void clean_hardfork_votes() = 0;
    virtual void push_hardfork_operation(const uint32_t hardfork) = 0;
};
}}
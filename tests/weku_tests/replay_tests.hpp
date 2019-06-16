#pragma once

#include <boost/test/unit_test.hpp>
#include "main_tests.hpp"

BOOST_AUTO_TEST_SUITE(replay_tests)

BOOST_FIXTURE_TEST_CASE(head_block_num_should_be_0_before_replay_chain, x_database_fixture)
{
    BOOST_REQUIRE(db.head_block_num() == 0);
}

BOOST_FIXTURE_TEST_CASE(ledger_blocks_size_should_be_empty_before_replay_chain, x_database_fixture)
{
    BOOST_REQUIRE(db.ledger_blocks().size() == 0);
}

BOOST_FIXTURE_TEST_CASE(head_block_num_should_be_1_after_replay_chain_to_block_1, x_database_fixture)
{
    db.generate_block(); // prepare database

    db.replay_chain();
    BOOST_REQUIRE(db.head_block_num() == 1);
}

BOOST_FIXTURE_TEST_CASE(leger_blocks_size_should_be_1_after_replay_chain_to_block_1, x_database_fixture)
{
    db.generate_block(); // prepare database

    db.replay_chain();
    BOOST_REQUIRE(db.ledger_blocks().size() == 1);
}

BOOST_FIXTURE_TEST_CASE(head_block_num_should_be_match_after_replay_chain, x_database_fixture)
{
    const uint32_t to_block_num = 100;
    db.debug_generate_block_until(to_block_num); // prepare database

    db.replay_chain();
    BOOST_REQUIRE(db.head_block_num() == to_block_num - 1);
}

BOOST_FIXTURE_TEST_CASE(ledger_blocks_should_be_match_after_replay_chain, x_database_fixture)
{
    const uint32_t to_block_num = 100;
    db.debug_generate_block_until(to_block_num); // prepare database

    db.replay_chain();
    BOOST_REQUIRE(db.ledger_blocks().size() == to_block_num - 1);
}

BOOST_AUTO_TEST_SUITE_END()
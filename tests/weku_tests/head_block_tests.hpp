#pragma once

#include <boost/test/unit_test.hpp>
#include "main_tests.hpp"

BOOST_AUTO_TEST_SUITE(head_block_tests)

BOOST_AUTO_TEST_CASE(head_block_num_should_be_0_after_database_init)
{
    mock_database db;
    BOOST_REQUIRE(db.head_block_num() == 0);
}

BOOST_AUTO_TEST_CASE(head_block_num_should_be_0_after_genesis)
{
    mock_database db;
    db.init_genesis();
    BOOST_REQUIRE(db.head_block_num() == 0);
}

BOOST_FIXTURE_TEST_CASE(head_block_num_should_be_1_after_first_block, x_database_fixture)
{
    db.generate_block();
    BOOST_REQUIRE(db.head_block_num() == 1);
}

BOOST_FIXTURE_TEST_CASE(head_block_num_should_be_increased_for_each_generate_block_called, x_database_fixture)
{
    for(uint32_t i = 1; i <= 100; i++){
        db.generate_block();
        BOOST_REQUIRE(db.head_block_num() == i);
    }
}

BOOST_FIXTURE_TEST_CASE(head_block_num_should_be_increased_for_each_apply_block_called, x_database_fixture)
{
    for(uint32_t i = 1; i <= 100; i++){
        signed_block b = db.generate_block_before_apply();
        db.apply_block(b);
        BOOST_REQUIRE(db.head_block_num() == i);
    }
}

BOOST_FIXTURE_TEST_CASE(last_hardfork_should_be_0_before_replay_chain, x_database_fixture)
{
    BOOST_REQUIRE(db.last_hardfork() == 0);
}

BOOST_FIXTURE_TEST_CASE(last_hardfork_should_be_19_after_replay_chain_to_block_1, x_database_fixture)
{
     db.generate_block();

     db.replay_chain();
     BOOST_REQUIRE(db.last_hardfork() == 19);
}

BOOST_FIXTURE_TEST_CASE(hardfork_to_19_should_only_happen_once_during_replay, x_database_fixture)
{
     db.generate_block();
     db.replay_chain();
     db.generate_block();
     BOOST_REQUIRE(db.last_hardfork() == 19);
}

BOOST_FIXTURE_TEST_CASE(last_hardfork_should_be10_after_replay_chain_to_hardfork_20, x_database_fixture)
{
    db.debug_generate_block_until(HARDFORK_20_BLOCK_NUM + 1);
    
    db.replay_chain();
    BOOST_REQUIRE(db.last_hardfork() == 20);
}

BOOST_FIXTURE_TEST_CASE(hardfork_20_should_only_happen_once_during_replay, x_database_fixture)
{
    db.debug_generate_block_until(HARDFORK_20_BLOCK_NUM + 1);
    
    db.replay_chain();
    db.generate_block();
    BOOST_REQUIRE(db.last_hardfork() == 20);
}

BOOST_AUTO_TEST_SUITE_END()
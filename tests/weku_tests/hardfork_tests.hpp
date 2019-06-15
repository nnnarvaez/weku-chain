#pragma once

#include <boost/test/unit_test.hpp>
#include "new_clazz.hpp"

BOOST_AUTO_TEST_SUITE(hardfork_tests)

BOOST_AUTO_TEST_CASE(last_hardfork_should_be_0_after_database_init)
{
    x_database db;
    BOOST_REQUIRE(db.last_hardfork() == 0);
}

BOOST_AUTO_TEST_CASE(last_hardfork_should_be_0_after_genesis)
{
    x_database db;
    db.init_genesis();
    BOOST_REQUIRE(db.last_hardfork() == 0);
}

// comment out, because unit test should only test behaviour instead of function
// BOOST_FIXTURE_TEST_CASE(apply_hardfork_0_should_throw_exception, x_database_fixture)
// {
//     BOOST_REQUIRE_THROW(db.apply_hardfork(0), std::runtime_error);
// }

// BOOST_FIXTURE_TEST_CASE(apply_hardfork_should_increase_last_hardfork, x_database_fixture)
// {
//     for (uint32_t i = 1; i <= 22; i++)
//     {
//         db.apply_hardfork(i);
//         BOOST_REQUIRE(db.last_hardfork() == i);
//     }
// }

BOOST_FIXTURE_TEST_CASE(should_appied_to_hardfork_19_after_block_1, x_database_fixture)
{    
    db.generate_block();
    BOOST_REQUIRE(db.last_hardfork() == 19);
}

BOOST_FIXTURE_TEST_CASE(process_hardfork_until_19_should_only_happen_once, x_database_fixture)
{    
    db.generate_block();
    BOOST_REQUIRE(db.last_hardfork() == 19);
    db.generate_block();
    BOOST_REQUIRE(db.last_hardfork() == 19);
}

BOOST_FIXTURE_TEST_CASE(process_hardfork_20_should_update_last_hardfork_to_20, x_database_fixture)
{
    db.debug_generate_block_until(HARDFORK_20_BLOCK_NUM);
    BOOST_REQUIRE(db.head_block_num() == HARDFORK_20_BLOCK_NUM - 1);
    auto b = db.generate_block_before_apply();
    db.apply_block(b);
    BOOST_REQUIRE(db.last_hardfork() == 20);
}

BOOST_FIXTURE_TEST_CASE(process_hardfork_21_should_update_last_hardfork_to_21, x_database_fixture)
{
    db.debug_generate_block_until(HARDFORK_21_BLOCK_NUM);
    BOOST_REQUIRE(db.head_block_num() == HARDFORK_21_BLOCK_NUM - 1);
    auto b = db.generate_block_before_apply();
    db.apply_block(b);
    BOOST_REQUIRE(db.last_hardfork() == 21);
}

BOOST_FIXTURE_TEST_CASE(hardfork_22_should_not_be_triggered_while_no_votes, x_database_fixture)
{
    db.debug_generate_block_until(HARDFORK_22_BLOCK_NUM + 1); 
    BOOST_REQUIRE(db.last_hardfork() == 21);
}

BOOST_FIXTURE_TEST_CASE(hardfork_22_should_not_be_triggered_before_votes_less_than_17, x_database_fixture)
{
    db.debug_generate_block_until(HARDFORK_22_BLOCK_NUM); 
    hardfork_votes_type votes;
    for(int32_t i = 0; i < REQUIRED_HARDFORK_VOTES - 1; i++) // vote for hf22 16 times
    {
        auto hardfork_and_block_num = std::make_pair(HARDFORK_22, HARDFORK_22_BLOCK_NUM);
        votes.push_back(hardfork_and_block_num);
    }    
    db.debug_set_next_hardfork_votes(votes);

    db.generate_block();

    BOOST_REQUIRE(db.last_hardfork() == 21);
}

BOOST_FIXTURE_TEST_CASE(hardfork_22_should_be_triggered_after_votes_greater_or_equal_than_17, x_database_fixture)
{
    db.debug_generate_block_until(HARDFORK_22_BLOCK_NUM); 
    hardfork_votes_type votes;
    for(int32_t i = 0; i < REQUIRED_HARDFORK_VOTES; i++) // vote for hf22 17 times
    {
        auto hardfork_and_block_num = std::make_pair(HARDFORK_22, HARDFORK_22_BLOCK_NUM);
        votes.push_back(hardfork_and_block_num);
    }    
    db.debug_set_next_hardfork_votes(votes);

    db.generate_block();
    BOOST_REQUIRE(db.last_hardfork() == 22);
}

BOOST_AUTO_TEST_SUITE_END()
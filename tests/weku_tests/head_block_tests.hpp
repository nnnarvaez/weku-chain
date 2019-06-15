#pragma once

#include <boost/test/unit_test.hpp>
#include "new_clazz.hpp"

BOOST_AUTO_TEST_SUITE(head_block_tests)

BOOST_AUTO_TEST_CASE(head_block_num_should_be_0_after_database_init)
{
    x_database db;
    BOOST_REQUIRE(db.head_block_num() == 0);
}

BOOST_AUTO_TEST_CASE(head_block_num_should_be_0_after_genesis)
{
    x_database db;
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

BOOST_AUTO_TEST_SUITE_END()
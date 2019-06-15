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

BOOST_FIXTURE_TEST_CASE(apply_hardfork_0_should_throw_exception, x_database_fixture)
{
    BOOST_REQUIRE_THROW(db.apply_hardfork(0), std::runtime_error);
}

BOOST_FIXTURE_TEST_CASE(apply_hardfork_should_increase_last_hardfork, x_database_fixture)
{
    for (uint32_t i = 1; i <= 22; i++)
    {
        db.apply_hardfork(i);
        BOOST_REQUIRE(db.last_hardfork() == i);
    }
}

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

BOOST_FIXTURE_TEST_CASE(process_hardfork_20, x_database_fixture)
{
    db.generate_block_until(HARDFORK_20_BLOCK_NUM);
    BOOST_REQUIRE(db.last_hardfork() == 20);
}

BOOST_AUTO_TEST_SUITE_END()
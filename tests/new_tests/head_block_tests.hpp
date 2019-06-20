#pragma once

#include <boost/test/unit_test.hpp>
#include "main_tests.hpp"

class mock_database:public database{

};

BOOST_AUTO_TEST_SUITE(head_block_tests)

BOOST_AUTO_TEST_CASE(head_block_num_should_be_0_after_database_init)
{
    mock_database db;
    BOOST_REQUIRE(db.head_block_num() == 0);
}

BOOST_AUTO_TEST_SUITE_END()
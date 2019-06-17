#pragma once

#include <wk/chain_new/mock_database.hpp>

using namespace wk::chain;

struct x_database_fixture{
    mock_database db;

    x_database_fixture()
    {
        //std::cout << "test suite setup.";
        db.init_genesis();
    };
    ~x_database_fixture()
    {
        //std::cout << "test suite teardown.";
    };
};
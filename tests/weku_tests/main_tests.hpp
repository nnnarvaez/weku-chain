#pragma once

#include <wk/chain_new/x_database.hpp>

using namespace wk::chain_new;

struct x_database_fixture{
    x_database db;

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
#pragma once

#include <cstdlib>

#define HARDFORK_20 20u
#define HARDFORK_20_BLOCK_NUM 200u

struct signed_block
{
};

class x_database
{
public:
    void init_genesis();
    uint32_t head_block_num(); 
    uint32_t last_hardfork();  

    void process_hardforks_until_19(); 
    void apply_hardfork(uint32_t hardfork);
    bool apply_block(const signed_block&);
    signed_block generate_block_before_apply();
    signed_block generate_block();
    signed_block generate_block_until(uint32_t hardfork_time);
private:
    uint32_t _head_block_num = 0; // TODO: ADD UNIT TEST FOR THIS
    uint32_t _last_hardfork = 0;
};

void x_database::init_genesis()
{
}

uint32_t x_database::head_block_num(){
    return _head_block_num;
}

uint32_t x_database::last_hardfork()
{
    return _last_hardfork;
}

void x_database::process_hardforks_until_19(){
    _last_hardfork = 19;
}

void x_database::apply_hardfork(uint32_t hardfork)
{
    if (hardfork == 0)
        throw std::runtime_error("cannot apply hardfork 0");
    _last_hardfork = hardfork;
}

bool x_database::apply_block(const signed_block& b){

    _head_block_num++;

    process_hardforks_until_19();
    if(head_block_num() == HARDFORK_20_BLOCK_NUM)
        _last_hardfork = HARDFORK_20;
    return true;
}

signed_block x_database::generate_block_before_apply()
{
    signed_block b;
    return b;
}

signed_block x_database::generate_block()
{
    signed_block b = generate_block_before_apply();
    apply_block(b);    
    return b;
}

signed_block x_database::generate_block_until(uint32_t block_num)
{
    signed_block b;
    for(uint32_t i = 1; i <= block_num; i++)
        generate_block();
    return b;
}

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
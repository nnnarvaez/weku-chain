#include <boost/test/unit_test.hpp>

struct signed_block{};

class database{
    public:
    uint32_t get_last_hardfork();
    void init_genesis();
    signed_block generate_block();
    void apply_hardfork(uint32_t hardfork);

    //bool has_hardfork(uint32_t hardfork);
    //void process_hardforks();

    private:
    uint32_t _last_hardfork = 0;
};

uint32_t database::get_last_hardfork(){
    return _last_hardfork;
}

void database::init_genesis(){

}

signed_block database::generate_block(){
    signed_block b;
    return b;
}

void database::apply_hardfork(uint32_t hardfork){
    if(hardfork == 0) throw std::runtime_error("cannot apply hardfork 0");;
    _last_hardfork = hardfork;
}

/* 
bool database::has_hardfork(uint32_t hardfork){
    return false;
}

void database::process_hardforks(){

}*/

BOOST_AUTO_TEST_SUITE(hardfork_tests)

BOOST_AUTO_TEST_CASE(test_db_hardfork_init){
    database db;
    BOOST_REQUIRE(db.get_last_hardfork() == 0);
}

BOOST_AUTO_TEST_CASE(test_db_hardfork_after_genesis){
    database db;
    db.init_genesis();
    BOOST_REQUIRE(db.get_last_hardfork() == 0);
}

BOOST_AUTO_TEST_CASE(test_apply_hardfork_0_should_throw_exception){
    database db;
    db.init_genesis();
    // should throw exception if system try to apply hardfork 0
    BOOST_REQUIRE_THROW(db.apply_hardfork(0), std::runtime_error); 
}

BOOST_AUTO_TEST_CASE(test_apply_hardfork){
    database db;
    db.init_genesis();    
    
    for(uint32_t i = 1; i <= 22; i++){
        db.apply_hardfork(i);
        BOOST_REQUIRE(db.get_last_hardfork() == i);
    }
}

BOOST_AUTO_TEST_CASE(test_should_appied_to_hardfork_19_after_block_1){
    database db;
    db.init_genesis();
    db.generate_block();
    BOOST_REQUIRE(db.get_last_hardfork() == 19);
}

/* 
BOOST_AUTO_TEST_CASE(test_process_hardforks){
    database db;
    db.has_hardfork(0);
    db.process_hardforks();
    db.has_hardfork(1);
}*/

BOOST_AUTO_TEST_SUITE_END()
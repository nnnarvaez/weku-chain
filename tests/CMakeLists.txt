include_directories(/usr/local/Cellar/boost@1.60/1.60.0/include)

#file(GLOB COMMON_SOURCES "common/*.cpp")

#find_package( Gperftools QUIET )
#if( GPERFTOOLS_FOUND )
#    message( STATUS "Found gperftools; compiling tests with TCMalloc")
#    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
#endif()

#file(GLOB UNIT_TESTS "tests/*.cpp")
#add_executable( chain_test ${UNIT_TESTS} ${COMMON_SOURCES} )
#target_link_libraries( chain_test chainbase steemit_chain steemit_protocol steemit_app steemit_account_history steemit_market_history steemit_witness steemit_debug_node fc ${PLATFORM_SPECIFIC_LIBS} )

#file(GLOB PLUGIN_TESTS "plugin_tests/*.cpp")
#add_executable( plugin_test ${PLUGIN_TESTS} ${COMMON_SOURCES} )
#target_link_libraries( plugin_test steemit_chain steemit_protocol steemit_app steemit_account_history steemit_market_history steemit_witness steemit_debug_node fc ${PLATFORM_SPECIFIC_LIBS} )

#file(GLOB HARDFORK_TESTS "hardfork/*.cpp")
#add_executable( hardfork_test ${HARDFORK_TESTS} ${COMMON_SOURCES} )
#target_link_libraries( hardfork_test  chainbase steemit_chain steemit_protocol steemit_app steemit_account_history steemit_market_history steemit_witness steemit_debug_node fc ${PLATFORM_SPECIFIC_LIBS} )

#file(GLOB WEKU_TESTS "weku_tests/*.cpp")
#add_executable( weku_test ${WEKU_TESTS} ${COMMON_SOURCES} )
#target_link_libraries( weku_test chain_mock ${PLATFORM_SPECIFIC_LIBS} )

file(GLOB NEW_TESTS "new_tests/*.cpp")
add_executable( new_tests ${NEW_TESTS})
target_link_libraries( new_tests gtest gmock chain ${PLATFORM_SPECIFIC_LIBS} )

#file(GLOB INTEGRATION_TESTS "integration/*.cpp")
#add_executable( integration_tests ${INTEGRATION_TESTS})
#target_link_libraries( integration_tests gtest gmock chain chain_shared ${PLATFORM_SPECIFIC_LIBS} )

#if(MSVC)
#  set_source_files_properties( tests/serialization_tests.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
#endif(MSVC)

#add_subdirectory( generate_empty_blocks )

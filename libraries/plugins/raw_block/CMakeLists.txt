file(GLOB HEADERS "include/weku/plugins/raw_block/*.hpp")

add_library( weku_raw_block
             ${HEADERS}
             raw_block_plugin.cpp
             raw_block_api.cpp
           )

target_link_libraries( weku_raw_block weku_app )
target_include_directories( weku_raw_block
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

file(GLOB HEADERS "include/weku/plugins/block_info/*.hpp")

add_library( weku_block_info
             ${HEADERS}
             block_info_plugin.cpp
             block_info_api.cpp
           )

target_link_libraries( weku_block_info weku_app )
target_include_directories( weku_block_info
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

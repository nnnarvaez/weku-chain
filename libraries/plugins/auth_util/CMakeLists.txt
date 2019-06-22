file(GLOB HEADERS "include/weku/plugins/auth_util/*.hpp")

add_library( weku_auth_util
             ${HEADERS}
             auth_util_plugin.cpp
             auth_util_api.cpp
           )

target_link_libraries( weku_auth_util weku_app )
target_include_directories( weku_auth_util
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

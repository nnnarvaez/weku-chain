file(GLOB HEADERS "include/weku/account_history/*.hpp")

add_library( weku_account_history
             account_history_plugin.cpp
           )

target_link_libraries( weku_account_history weku_app )
target_include_directories( weku_account_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   weku_account_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

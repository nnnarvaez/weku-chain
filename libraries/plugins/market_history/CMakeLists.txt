file(GLOB HEADERS "include/weku/market_history/*.hpp")

add_library( weku_market_history
             market_history_plugin.cpp
             market_history_api.cpp
           )

target_link_libraries( weku_market_history weku_app )
target_include_directories( weku_market_history
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   weku_market_history

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
file(GLOB HEADERS "include/weku/private_message/*.hpp")

add_library( weku_private_message
             private_message_plugin.cpp
           )

target_link_libraries( weku_private_message  weku_app )
target_include_directories( weku_private_message
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   weku_private_message

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

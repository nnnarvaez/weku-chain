file(GLOB HEADERS "include/weku/tags/*.hpp")

add_library( weku_tags
             tags_plugin.cpp )

target_link_libraries( weku_tags chain weku_app )
target_include_directories( weku_tags
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   weku_tags

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

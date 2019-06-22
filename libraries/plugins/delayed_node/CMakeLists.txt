file(GLOB HEADERS "include/weku/delayed_node/*.hpp")

add_library( weku_delayed_node
             delayed_node_plugin.cpp
           )

target_link_libraries( weku_delayed_node weku_app )
target_include_directories( weku_delayed_node
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

if(MSVC)
  set_source_files_properties( delayed_node_plugin.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
   weku_delayed_node

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

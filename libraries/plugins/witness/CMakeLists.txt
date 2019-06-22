file(GLOB HEADERS "include/weku/witness/*.hpp")

add_library( weku_witness
             witness_plugin.cpp
             witness_evaluators.cpp
             witness_operations.cpp
           )

target_link_libraries( weku_witness chain weku_app )
target_include_directories( weku_witness
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" )

install( TARGETS
   weku_witness

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)

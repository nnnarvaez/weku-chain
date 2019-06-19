file(GLOB HEADERS "include/weku/chain/*.hpp")
file(GLOB HEADERS2 "include/chainbase/*.hpp")
file(GLOB SRCS "*.cpp")
add_library( chain_objects ${SRCS} ${HEADERS} ${HEADERS2})

add_dependencies( chain_objects protocol chainbase)
target_link_libraries( chain_objects protocol chainbase fc ${PATCH_MERGE_LIB} )
target_include_directories( chain_objects PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   chain_objects

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/weku/chain_objects" )

file(GLOB HEADERS "include/weku/chain/*.hpp")
file(GLOB SRCS "*.cpp")
add_library( weku_chain_ledger ${SRCS} ${HEADERS} )

add_dependencies( weku_chain_ledger steemit_protocol chainbase fc)
target_link_libraries( weku_chain_ledger steemit_protocol chainbase fc ${PATCH_MERGE_LIB} )
target_include_directories( weku_chain_ledger PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   weku_chain_ledger

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/weku/chain" )

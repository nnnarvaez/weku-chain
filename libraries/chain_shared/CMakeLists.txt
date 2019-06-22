file(GLOB HEADERS "include/weku/chain/*.hpp")
file(GLOB SRCS "*.cpp")
add_library( chain_shared ${SRCS} ${HEADERS} )

add_dependencies( chain_shared chain_ledger chain_branch)
target_link_libraries( chain_shared chain_ledger chain_branch ${PATCH_MERGE_LIB} )
target_include_directories( chain_shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   chain_shared

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/weku/chain" )

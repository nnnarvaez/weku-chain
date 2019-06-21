include_directories(../fc/include)
file(GLOB HEADERS "include/weku/chain/*.hpp")
file(GLOB SRCS "*.cpp")
add_library( chain_ledger ${SRCS} ${HEADERS} )

add_dependencies( chain_ledger protocol )
target_link_libraries( chain_ledger protocol ${PATCH_MERGE_LIB} )
target_include_directories( chain_ledger PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   chain_ledger

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/weku/chain" )

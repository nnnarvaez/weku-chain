file(GLOB HEADERS "include/weku/chain/*.hpp")
file(GLOB SRCS "*.cpp")
add_library( chain_hardfork ${SRCS} ${HEADERS} )

add_dependencies( chain_hardfork chain_shared)
target_link_libraries( chain_hardfork chain_shared ${PATCH_MERGE_LIB} )
target_include_directories( chain_hardfork PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   chain_hardfork

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/weku/chain" )

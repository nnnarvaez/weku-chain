file(GLOB HEADERS "include/weku/chain/*.hpp")
file(GLOB SRCS "*.cpp")
add_library( chain_mock ${SRCS} ${HEADERS} )

add_dependencies( chain_mock chain )
target_link_libraries( chain_mock chain ${PATCH_MERGE_LIB} )
target_include_directories( chain_mock PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   chain_mock

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/weku/chain" )

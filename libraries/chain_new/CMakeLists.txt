file(GLOB HEADERS "include/wk/chain_new/*.hpp")
file(GLOB SRCS "*.cpp")
add_library( wk_chain_new ${SRCS} ${HEADERS} )

add_dependencies( wk_chain_new steemit_protocol )
target_link_libraries( wk_chain_new steemit_protocol fc chainbase graphene_schema ${PATCH_MERGE_LIB} )
target_include_directories( wk_chain_new PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   wk_chain_new

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/wk/chain_new" )

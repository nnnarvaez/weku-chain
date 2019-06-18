include_directories(../fc/include)

file(GLOB HEADERS "include/steemit/chain_objects/*.hpp")
file(GLOB SRCS "*.cpp")
add_library( steem_chain_objects ${SRCS} ${HEADERS} )

add_dependencies( steem_chain_objects steemit_protocol )
target_link_libraries( steem_chain_objects steemit_protocol fc chainbase graphene_schema ${PATCH_MERGE_LIB} )
target_include_directories( steem_chain_objects PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   steem_chain_objects

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/steemit/chain_objects" )

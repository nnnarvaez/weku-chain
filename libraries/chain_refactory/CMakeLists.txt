include_directories(../chain/include)
include_directories(../chain_util/include)
include_directories(../fc/include)

file(GLOB HEADERS "include/weku/chain/*.hpp")
file(GLOB SRCS "*.cpp")
add_library( chain_refactory ${SRCS} ${HEADERS} )

add_dependencies( chain_refactory chain_util chain_branch chain_objects chain_ledger protocol )
target_link_libraries( chain_refactory chain_util chain_branch chain_objects chain_ledger protocol fc chainbase graphene_schema ${PATCH_MERGE_LIB} )
target_include_directories( chain_refactory PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   chain_refactory

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/weku/chain" )

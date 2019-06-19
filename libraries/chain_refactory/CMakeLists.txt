include_directories(../chain/include)
include_directories(../chain_util/include)
include_directories(../fc/include)

file(GLOB HEADERS "include/weku/chain/*.hpp")
file(GLOB SRCS "*.cpp")
add_library( weku_chain_refactory ${SRCS} ${HEADERS} )

add_dependencies( weku_chain_refactory weku_chain_util weku_chain_objects weku_chain_ledger steemit_protocol )
target_link_libraries( weku_chain_refactory weku_chain_util weku_chain_objects weku_chain_ledger steemit_protocol fc chainbase graphene_schema ${PATCH_MERGE_LIB} )
target_include_directories( weku_chain_refactory PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   weku_chain_refactory

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/weku/chain" )

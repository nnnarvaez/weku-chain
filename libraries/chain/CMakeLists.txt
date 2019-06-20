file(GLOB HEADERS "include/weku/chain/*.hpp")
file(GLOB SRCS "*.cpp")

add_library( chain ${SRCS} ${HEADERS} )

add_dependencies( chain chain_refactory chain_shared )
target_link_libraries( chain chain_refactory chain_branch chain_ledger chain_shared chain_objects chainbase protocol fc chainbase graphene_schema ${PATCH_MERGE_LIB} )
target_include_directories( chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if(MSVC)
  set_source_files_properties( database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/weku/chain" )

file(GLOB HEADERS "include/weku/chain/*.hpp")
file(GLOB SRCS "*.cpp")

add_library( weku_chain ${SRCS} ${HEADERS} )

add_dependencies( weku_chain chain_refactory protocol )
target_link_libraries( weku_chain chain_refactory protocol fc chainbase graphene_schema ${PATCH_MERGE_LIB} )
target_include_directories( weku_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if(MSVC)
  set_source_files_properties( database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   weku_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/weku/chain" )

file(GLOB HEADERS "include/steemit/chain/*.hpp")
file(GLOB HEADERS2 "include/wk/chain_refactory/*.hpp")
file(GLOB SRCS "*.cpp")

## SORT .cpp by most likely to change / break compile
add_library( steemit_chain
             ${SRCS}
             ${HEADERS2}
             ${HEADERS}
           )

add_dependencies( steemit_chain wk_chain_refactory steemit_protocol )
target_link_libraries( steemit_chain wk_chain_refactory steemit_protocol fc chainbase graphene_schema ${PATCH_MERGE_LIB} )
target_include_directories( steemit_chain
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

if(MSVC)
  set_source_files_properties( database.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

INSTALL( TARGETS
   steemit_chain

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/steemit/chain" )

file(GLOB HEADERS "include/steemit/chain_new/*.hpp")

## SORT .cpp by most likely to change / break compile
add_library( steemit_chain_new
            # As database takes the longest to compile, start it first
            x_database.cpp
            hardfork_processor.cpp
             
            ${HEADERS}
           )

#add_dependencies( steemit_chain_new steemit_protocol )
#target_link_libraries( steemit_chain_new steemit_protocol fc chainbase graphene_schema ${PATCH_MERGE_LIB} )
target_include_directories( steemit_chain_new
                            PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   steemit_chain_new

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/steemit/chain_new" )

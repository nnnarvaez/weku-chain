file(GLOB HEADERS "include/weku/chain/*.hpp")
file(GLOB SRCS "*.cpp")
add_library( chain_branch ${SRCS} ${HEADERS} )

add_dependencies(chain_branch chain_objects)
target_link_libraries(chain_branch chain_objects ${PATCH_MERGE_LIB} )
target_include_directories(chain_branch PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include" "${CMAKE_CURRENT_BINARY_DIR}/include" )

INSTALL( TARGETS
   chain_branch

   RUNTIME DESTINATION bin
   LIBRARY DESTINATION lib
   ARCHIVE DESTINATION lib
)
INSTALL( FILES ${HEADERS} DESTINATION "include/weku/chain" )

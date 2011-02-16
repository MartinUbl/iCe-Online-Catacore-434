file(GLOB_RECURSE sources *.cpp *.h)

# uncomment next line to disable debug mode
add_definitions("-DIOMAP_DEBUG")

# build setup currently only supports libmpq 0.4.x
add_definitions("-DUSE_LIBMPQ04")
add_definitions("-Wall")
add_definitions("-ggdb")
add_definitions("-O3")

if( UNIX )
  include_directories(
    ${CMAKE_SOURCE_DIR}/dep/libmpq
  )
elseif( WIN32 )
  include_directories(
    ${CMAKE_SOURCE_DIR}/dep/libmpq
    ${CMAKE_SOURCE_DIR}/dep/libmpq/win
  )
endif()

add_executable(vmap3extractor ${sources})

target_link_libraries(vmap3extractor
  ${BZIP2_LIBRARIES}
  ${ZLIB_LIBRARIES}
  mpq
)

add_dependencies(vmap3extractor mpq)

if( UNIX )
  install(TARGETS vmap3extractor DESTINATION bin)
elseif( WIN32 )
  install(TARGETS vmap3extractor DESTINATION "${CMAKE_INSTALL_PREFIX}")
endif()

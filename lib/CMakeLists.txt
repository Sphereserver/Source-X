
add_subdirectory(bcrypt)
add_subdirectory(cdrc)
add_subdirectory(flat_containers)
add_subdirectory(mariadb_connector_c)
add_subdirectory(object_ptr)
add_subdirectory(parallel_hashmap)
add_subdirectory(rand)
add_subdirectory(regex)
#add_subdirectory(simde)
add_subdirectory(twofish)

if (${LIB_LIBEV_BUILD})
  add_subdirectory(libev)
# else find_package...
endif()
if (${LIB_SQLITE_BUILD})
  add_subdirectory(sqlite)
# else find_package...
endif()
if (${LIB_ZLIB_BUILD})
  add_subdirectory(zlib)
# else find_package...
endif()


foreach (tgt ${TARGETS})
  # Link to projects defined as interface libraries.
  target_link_libraries(${tgt}
    INTERFACE flat_containers     # header only lib
    INTERFACE mariadb_connector_c # true interface for the precompiled external library
    INTERFACE object_ptr          # header only lib
    INTERFACE parallel_hashmap    # header only lib
    INTERFACE rand                # header only lib
    INTERFACE regex               # header only lib
  )

  # workaround, INCLUDE_DIRECTORIES aren't propagated to projects linking interface libraries...
  target_include_directories(${tgt}
    PRIVATE $<TARGET_PROPERTY:flat_containers,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE $<TARGET_PROPERTY:mariadb_connector,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE $<TARGET_PROPERTY:object_ptr,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE $<TARGET_PROPERTY:parallel_hashmap,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE $<TARGET_PROPERTY:rand,INTERFACE_INCLUDE_DIRECTORIES>
    PRIVATE $<TARGET_PROPERTY:regex,INTERFACE_INCLUDE_DIRECTORIES>
  )

  # import the compile_definitions defined by the INTERFACE targets
  target_compile_definitions(${tgt} PUBLIC ${INTERFACE_COMPILE_DEFINITIONS})

  # Link to (and import include directories) those clasic (.c/.h) libraries.
  target_link_libraries(${tgt} PRIVATE bcrypt twofish) # cdrc (not needed for now)
  if (${LIB_LIBEV_BUILD})
    target_link_libraries(${tgt} PRIVATE libev)
  endif()
  if (${LIB_SQLITE_BUILD})
    target_link_libraries(${tgt} PRIVATE sqlite)
  endif()
  if (${LIB_ZLIB_BUILD})
    target_link_libraries(${tgt} PRIVATE zlib)
  endif()
endforeach ()


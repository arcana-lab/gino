if(EXISTS ${GINO_CMAKE_ROOT}/.git/HEAD)
  execute_process(
    COMMAND git remote get-url --all origin
    WORKING_DIRECTORY ${GINO_CMAKE_ROOT}
    OUTPUT_VARIABLE gino_config_GIT_ORIGIN
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
  )
  execute_process(
    COMMAND git log -1 --format=%H
    WORKING_DIRECTORY ${GINO_CMAKE_ROOT}
    OUTPUT_VARIABLE gino_config_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
  )
else()
  set(gino_config_GIT_ORIGIN "")
  set(gino_config_GIT_COMMIT "")
endif()

string(REPLACE ";" " " gino_config_CXX_FLAGS "${GINO_CXX_FLAGS}")

get_target_property(GINO_LIBRARIES gino_libraries NAMES)
set(gino_config_CORE_LIBS "")
foreach(lib IN LISTS GINO_LIBRARIES)
  list(APPEND gino_config_CORE_LIBS 
    "-load ${CMAKE_INSTALL_PREFIX}/lib/${lib}.so"
  )
endforeach()
string(REPLACE ";" " " gino_config_CORE_LIBS "${gino_config_CORE_LIBS}")

get_target_property(GINO_TOOL_LIBRARIES gino_tool_libraries NAMES)
set(gino_config_TOOL_LIBS "")
foreach(lib IN LISTS GINO_TOOL_LIBRARIES)
  list(APPEND gino_config_TOOL_LIBS
    "-load ${CMAKE_INSTALL_PREFIX}/lib/${lib}.so"
  )
endforeach()
string(REPLACE ";" " " gino_config_TOOL_LIBS "${gino_config_TOOL_LIBS}")

configure_file(gino-config.in gino-config @ONLY)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/gino-config DESTINATION bin)

add_subdirectory(core)

if(GINO_TOOLS STREQUAL ON)
  add_subdirectory(tools)
endif()

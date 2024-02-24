if(EXISTS ${GINO_CMAKE_ROOT}/.git/HEAD)
  execute_process(
    COMMAND git remote get-url --all origin
    WORKING_DIRECTORY ${GINO_CMAKE_ROOT}
    OUTPUT_VARIABLE GINO_GIT_ORIGIN
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
  )
  execute_process(
    COMMAND git log -1 --format=%H
    WORKING_DIRECTORY ${GINO_CMAKE_ROOT}
    OUTPUT_VARIABLE GINO_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
  )
else()
  set(GINO_GIT_ORIGIN "")
  set(GINO_GIT_COMMIT "")
endif()

string(REPLACE ";" " " GINO_CXX_FLAGS "${GINO_CXX_FLAGS}")
configure_file(src/config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/config.hpp @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR})
add_executable(gino-config src/Gino_config.cpp)

install(
  TARGETS gino-config
  DESTINATION bin
)

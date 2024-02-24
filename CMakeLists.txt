cmake_minimum_required(VERSION 3.4.3)

project(gino)

# See `cmake --help-policy CMP0077`
# `OLD` specifies that the effects of this policy are whatever LLVM 9
# assumed at that time.
cmake_policy(SET CMP0077 OLD)

set(GINO_CXX_FLAGS 
  -O0
  -g
  -fPIC
  -std=c++17
  -Wall
)

if(NOT DEFINED NOELLE_INSTALL_DIR OR (NOELLE_INSTALL_DIR STREQUAL ""))
  message(FATAL_ERROR "Missing value for NOELLE_INSTALL_DIR")
endif()

message(STATUS "NOELLE installed at ${NOELLE_INSTALL_DIR}")

set(GINO_CMAKE_ROOT ${CMAKE_CURRENT_SOURCE_DIR})

get_filename_component(
  CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX} REALPATH
)
get_filename_component(
  GINO_CMAKE_ROOT ${GINO_CMAKE_ROOT} REALPATH
)

file(READ ${GINO_CMAKE_ROOT}/VERSION GINO_VERSION)
string(STRIP ${GINO_VERSION} GINO_VERSION)
set(GINO_EXTERNAL_DIR ${GINO_CMAKE_ROOT}/external)

configure_file(enable.in enable @ONLY)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/enable
  DESTINATION ${GINO_CMAKE_ROOT}
)

install(
  FILES ${GINO_CMAKE_ROOT}/.githooks/pre-commit
  DESTINATION ${GINO_CMAKE_ROOT}/.git/hooks
  PERMISSIONS
    OWNER_EXECUTE OWNER_WRITE OWNER_READ
    GROUP_EXECUTE GROUP_READ
    WORLD_EXECUTE WORLD_READ
)

enable_language(C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(LLVM 9 REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
link_directories(${LLVM_LIBRARY_DIRS})

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# prepare the pass to be included in the source tree
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(AddLLVM)
include(HandleLLVMOptions)

include_directories(
  ${LLVM_INCLUDE_DIRS}
  ${NOELLE_INSTALL_DIR}/include
)

set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/)
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/)

add_compile_options(${GINO_CXX_FLAGS})

add_definitions(
  -D__STDC_LIMIT_MACROS
  -D__STDC_CONSTANT_MACROS
)

add_custom_target(
  CompileCommands ALL
  COMMAND ${CMAKE_COMMAND} -E create_symlink
    ${CMAKE_BINARY_DIR}/compile_commands.json
    ${GINO_CMAKE_ROOT}/compile_commands.json
)

macro(add_gino_component name)
  add_custom_target(${name})
  set_target_properties(${name} PROPERTIES SOURCES "")
endmacro()

macro(add_gino_component_sources component)
  cmake_parse_arguments(ARG "" "" "FILES" ${ARGV})
  set(files "")
  foreach(file IN LISTS ARG_FILES)
    set(file "${CMAKE_CURRENT_LIST_DIR}/${file}")
    list(APPEND files ${file})
  endforeach()
  get_target_property(sources ${component} SOURCES)
  list(APPEND sources "${files}")
  set_target_properties(${component} PROPERTIES SOURCES "${sources}")
endmacro()

add_subdirectory(src)

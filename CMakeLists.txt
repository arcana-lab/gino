cmake_minimum_required(VERSION 3.13)

project(gino)

include(FetchContent)

include(config.default.cmake)
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/config.cmake)
  include(config.cmake)
endif()

if(DEFINED GINO_INSTALL_DIR)
  set(CMAKE_INSTALL_PREFIX ${GINO_INSTALL_DIR})
endif()

set(GINO_CXX_FLAGS 
  -O0
  -g
  -fPIC
  -std=c++17
  -Wall
)

string(ASCII 27 Esc)
set(Purple "${Esc}[35m")
set(Green "${Esc}[32m")
set(Red "${Esc}[31m")
set(ColorReset "${Esc}[m")

function(gino_acquire_option OPT)
  if(${OPT} MATCHES "ON|On|on|1")
    set(${OPT} ON PARENT_SCOPE)
    message(STATUS "${Purple}Option${ColorReset} ${OPT} ${Green}ON${ColorReset}")
  elseif(${OPT} MATCHES "OFF|Off|off|0")
    set(${OPT} OFF PARENT_SCOPE)
    message(STATUS "${Purple}Option${ColorReset} ${OPT} ${Red}OFF${ColorReset}")
  elseif(${OPT} MATCHES "OFF|Off|off|0")
  else()
    message(FATAL_ERROR "${OPT} must be either ON or OFF")
  endif()
endfunction()

message(STATUS "${Purple}Install directory${ColorReset} is ${CMAKE_INSTALL_PREFIX}")

gino_acquire_option(GINO_TOOLS)

if(NOT DEFINED NOELLE_INSTALL_DIR OR (NOELLE_INSTALL_DIR STREQUAL ""))
  message(FATAL_ERROR "Missing value for NOELLE_INSTALL_DIR")
endif()

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
  PROGRAMS ${GINO_CMAKE_ROOT}/.githooks/pre-commit
  DESTINATION ${GINO_CMAKE_ROOT}/.git/hooks
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

add_custom_target(gino_libraries)
add_custom_target(gino_tool_libraries)
set_target_properties(gino_libraries PROPERTIES NAMES "")
set_target_properties(gino_tool_libraries PROPERTIES NAMES "")

function(gino_component_declare name)
  add_llvm_library(${name} MODULE)
  get_target_property(names gino_libraries NAMES)
  list(APPEND names ${name})
  set_target_properties(gino_libraries PROPERTIES NAMES "${names}")
endfunction()

function(gino_tool_declare name)
  add_llvm_library(${name} MODULE)
  get_target_property(names gino_tool_libraries NAMES)
  list(APPEND names ${name})
  set_target_properties(gino_tool_libraries PROPERTIES NAMES "${names}")
endfunction()

add_subdirectory(src)
add_subdirectory(bin)

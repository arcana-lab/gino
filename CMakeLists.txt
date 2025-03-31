cmake_minimum_required(VERSION 3.13)

project(Gino LANGUAGES C CXX)

# Set installation directories
set(CMAKE_INSTALL_LIBDIR ${CMAKE_INSTALL_PREFIX}/lib)
set(CMAKE_INSTALL_BINDIR ${CMAKE_INSTALL_PREFIX}/bin)
message(STATUS "Installation prefix: ${CMAKE_INSTALL_PREFIX}")

# Configure compiler
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -ggdb")

# Configure LLVM
find_package(LLVM 18.1.8 REQUIRED CONFIG)
set(LLVM_RUNTIME_OUTPUT_INTDIR ${CMAKE_INSTALL_LIBDIR})
set(LLVM_LIBRARY_OUTPUT_INTDIR ${CMAKE_INSTALL_LIBDIR})
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
include(HandleLLVMOptions)
include(AddLLVM)

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
add_definitions(
-D__STDC_LIMIT_MACROS
-D__STDC_CONSTANT_MACROS
)

message(STATUS "LLVM directory: ${LLVM_CMAKE_DIR}")

# Require noelle
find_program(NOELLE_CONFIG noelle-config REQUIRED)
execute_process(
  COMMAND noelle-config --prefix
  OUTPUT_VARIABLE NOELLE_INSTALL_PREFIX
  OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET
)
include_directories(${NOELLE_INSTALL_PREFIX}/include)
message(STATUS "Using Noelle: ${NOELLE_INSTALL_PREFIX}")


add_subdirectory(src)


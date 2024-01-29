# Project
cmake_minimum_required(VERSION 3.13)
project(DOALL)

# Dependences
include(${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/DependencesCMake.txt)

# Pass
add_subdirectory(src)

# Install
install(
  FILES
  include/arcana/gino/core/DOALL.hpp 
  include/arcana/gino/core/DOALLTask.hpp
  DESTINATION 
  include/arcana/gino/core
  )

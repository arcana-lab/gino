# Project
cmake_minimum_required(VERSION 3.13)
project(DSWP)

# Dependences
include(${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/DependencesCMake.txt)

# Pass
add_subdirectory(src)

# Install
install(
  FILES
  include/arcana/gino/core/DSWP.hpp 
  include/arcana/gino/core/DSWPTask.hpp 
  DESTINATION 
  include/arcana/gino/core
  )

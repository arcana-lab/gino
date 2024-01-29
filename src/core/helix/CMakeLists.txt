# Project
cmake_minimum_required(VERSION 3.13)
project(HELIX)

# Dependences
include(${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/DependencesCMake.txt)

# Pass
add_subdirectory(src)

# Install
install(
  FILES
  include/arcana/gino/core/HELIX.hpp 
  include/arcana/gino/core/HELIXTask.hpp 
  include/arcana/gino/core/SequentialSegment.hpp 
  include/arcana/gino/core/SpilledLoopCarriedDependence.hpp
  DESTINATION 
  include/arcana/gino/core
  )

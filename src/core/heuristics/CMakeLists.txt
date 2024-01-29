# Project
cmake_minimum_required(VERSION 3.13)
project(Heuristics)

# Dependences
include(${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/DependencesCMake.txt)

# Pass
add_subdirectory(src)

# Install
install(
  FILES
  include/arcana/gino/core/HeuristicsPass.hpp 
  include/arcana/gino/core/Heuristics.hpp
  include/arcana/gino/core/InvocationLatency.hpp
  include/arcana/gino/core/PartitionCostAnalysis.hpp
  include/arcana/gino/core/SmallestSizePartitionAnalysis.hpp
  include/arcana/gino/core/MinMaxSizePartitionAnalysis.hpp
  DESTINATION 
  include/arcana/gino/core
  )

# Project
cmake_minimum_required(VERSION 3.13)
project(ParallelizationTechnique)

# Dependences
include(${CMAKE_CURRENT_SOURCE_DIR}/../../scripts/DependencesCMake.txt)

# Pass
add_subdirectory(src)

# Install
install(
  FILES
  include/arcana/gino/core/ParallelizationTechnique.hpp 
  include/arcana/gino/core/ParallelizationTechniqueForLoopsWithLoopCarriedDataDependences.hpp 
  DESTINATION 
  include/arcana/gino/core
  )

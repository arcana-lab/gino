
add_llvm_library(Gino MODULE PARTIAL_SOURCES_INTENDED)

target_sources(Gino
  PRIVATE
  Passes.cpp)

add_subdirectory(core)

add_subdirectory(tools)

install(
  TARGETS Gino
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_LIBDIR})



add_llvm_library(Gino MODULE PARTIAL_SOURCES_INTENDED)

target_sources(Gino
  PRIVATE
  Passes.cpp)

add_subdirectory(core)

add_subdirectory(tools)

# Note: temporary solution.
# Hopefully this won't be needed if noelle is packed into a single .so file.
target_link_libraries(Gino
  PUBLIC
  ${NOELLE_INSTALL_PREFIX}/lib/LoopInvariantCodeMotion.so
  ${NOELLE_INSTALL_PREFIX}/lib/SCEVSimplification.so
  )

install(
  TARGETS Gino
  LIBRARY
  DESTINATION ${CMAKE_INSTALL_LIBDIR})


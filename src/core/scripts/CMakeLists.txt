
configure_file(enable.in enable @ONLY)
install(
  FILES
  ${CMAKE_CURRENT_BINARY_DIR}/enable
  DESTINATION
  ${CMAKE_SOURCE_DIR}
)

configure_file(gino-config.in gino-config @ONLY)
install(
  PROGRAMS
  ${CMAKE_CURRENT_BINARY_DIR}/gino-config
  TYPE BIN
)

install(
  PROGRAMS
  gino
  gino-autotuner
  gino-enable
  gino-inline
  gino-io
  gino-load
  gino-loops
  gino-planner
  gino-pre

  TYPE BIN
)


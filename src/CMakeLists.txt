set(GINO_CORE_MODULES
  autotuner_doall_filter
  autotuner_search_space
  doall
  dswp
  enablers
  helix
  heuristics
  inliner
  input_output
  parallelization_planner
  parallelization_technique
  parallelizer
)

set(GINO_TOOL_MODULES
  parallelizer_plan_info
  time_saved
)

if(GINO_ENABLE_REPL)
  list(APPEND GINO_TOOL_MODULES repl)
endif()

add_subdirectory(core)
add_subdirectory(tools)

gino_tool_declare(TimeSaved)
target_sources(
  TimeSaved # component name
  PRIVATE
  src/LoopEvaluation.cpp
  src/LoopSelector.cpp
  src/Pass.cpp
  src/TimingModel.cpp
)

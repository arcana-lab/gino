gino_tool_declare(ParallelizerPlanInfo)
target_sources(
  ParallelizerPlanInfo # component name
  PRIVATE
  src/Pass.cpp
  src/PlanInfo.cpp
)

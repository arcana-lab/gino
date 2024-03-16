target_sources(
  Gino # component name
  PRIVATE
  src/Heuristics.cpp
  src/HeuristicsPass.cpp
  src/InvocationLatency.cpp
  src/MinMaxSizePartitionAnalysis.cpp
  src/PartitionCostAnalysis.cpp
  src/SmallestSizePartitionAnalysis.cpp
)

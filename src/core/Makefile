ENABLERS=enablers input_output inliner
TUNING=autotuner_doall_filter autotuner_search_space
HEURISTICS=parallelization_planner heuristics parallelizer_plan_info time_saved
PARALLELIZATION_TECHNIQUES=doall helix dswp 
ALL=$(ENABLERS) $(HEURISTICS) $(PARALLELIZATION_TECHNIQUES) $(TUNING) parallelizer

all: $(ALL)

parallelizer:
	cd $@ ; ../../scripts/run_me.sh

enablers:
	cd $@ ; ../../scripts/run_me.sh

inliner:
	cd $@ ; ../../scripts/run_me.sh

input_output:
	cd $@ ; ../../scripts/run_me.sh

doall:
	cd $@ ; ../../scripts/run_me.sh

helix:
	cd $@ ; ../../scripts/run_me.sh

dswp:
	cd $@ ; ../../scripts/run_me.sh

parallelization_planner:
	cd $@ ; ../../scripts/run_me.sh

heuristics:
	cd $@ ; ../../scripts/run_me.sh

time_saved:
	cd $@ ; ../../scripts/run_me.sh

parallelizer_plan_info:
	cd $@ ; ../../scripts/run_me.sh

autotuner_doall_filter:
	cd $@ ; ../../scripts/run_me.sh

autotuner_search_space:
	cd $@ ; ../../scripts/run_me.sh

clean:
	rm -rf */build */*.json ; 
	rm -rf */build */*/*.json ; 

.PHONY: $(ALL) clean

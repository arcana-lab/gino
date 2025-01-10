ALL=parallelizer_plan_info time_saved loop_analyze

all: $(ALL) scripts

scripts:
	./scripts/installGINOTools.sh

parallelizer_plan_info:
	cd $@ ; ../../scripts/run_me.sh

loop_analyze:
	cd $@ ; ../../scripts/run_me.sh

time_saved:
	cd $@ ; ../../scripts/run_me.sh

unistall: clean

clean:
	rm -rf */build */*.json ; 
	rm -rf */build */*/*.json ; 

.PHONY: $(ALL) scripts clean uninstall

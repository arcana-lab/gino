RUNTIME_DIRNAME="threadpool"
RUNTIME_GITREPO="https://github.com/scampanoni/virgil.git"
RUNTIME_VERSION="2.0.0"

all: regression performance

condor: download
	source ../enable ; cd condor ; make ; make submit ;

condor_autotuner:
	cd condor ; make performance_autotuner.con ; make submit ;

condor_watch:
	./scripts/condor_watch.sh ;

condor_check: 
	./scripts/condor_check.sh ;

condor_regression_add:
	./scripts/condor_regression_set.sh ;

regression: download
	./scripts/test_regression.sh ;

performance: download
	./scripts/test_performance.sh ;

performance_autotuner: download
	./scripts/test_performance_autotuner.sh ;

download:
	mkdir -p include ; cd include ; ../scripts/download.sh "$(RUNTIME_GITREPO)" $(RUNTIME_VERSION) "$(RUNTIME_DIRNAME)" ;
	./scripts/add_symbolic_link.sh ;

clean:
	./scripts/clean.sh ; rm -rf include/ ; rm -f regression/*.txt ;
	rm -rf regression_* ;
	rm -rf tmp* ;
	cd condor ; make clean ; 
	rm -f compiler_output* ;
	find ./ -name output_parallelized.txt.xz -delete
	find ./ -name vgcore* -delete
	rm -f TestDir_not_exists*

.PHONY: condor condor_autotuner condor_check regression performance performance_autotuner download clean condor_regression_add

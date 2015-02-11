test-all: test-sidekick

test-sidekick:
	@echo -e "\n Probing 'time()'"
	../src/sidekick -c 1 -d 1 --time --verbose
	../src/sidekick -c 1 -d 1 --time --verbose --humanize
	@echo -e "\n Probing 'gettimeofday()'"
	../src/sidekick -c 1 -d 1 --gettimeofday --verbose
	../src/sidekick -c 1 -d 1 --gettimeofday --verbose --humanize
	@echo -e "\n Probing 'clock_gettime()'"
	../src/sidekick -c 1 -d 1 --clock_gettime --verbose
	../src/sidekick -c 1 -d 1 --clock_gettime --verbose --humanize

all: clean build test

clean:
	$(MAKE) -C src clean

build:
	$(MAKE) -C src build

test:
	$(MAKE) -C test all

all: build-tman build-test

clean:
	$(MAKE) -C src clean
	$(MAKE) -C test clean

build-tman:
	$(MAKE) -C src all

build-test:
	$(MAKE) -C test all

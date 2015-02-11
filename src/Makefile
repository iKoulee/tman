all: clean build

build: sidekick libtman.so tman

libtman.so:
	$(CC) -ansi -Wall --shared -ldl -lrt -fPIC -o $@ libtman.c

tman:
	$(CC) -ansi -Wall -lrt -lm -o $@ tman.c

sidekick: sidekick.o
	$(CC) -ansi -Wall -o $@ $<

sidekick.o: sidekick.c
	$(CC) -ansi -Wall -o $@ -c $<

clean:
	$(RM) ./libtman.so
	$(RM) ./tman
	$(RM) ./sidekick

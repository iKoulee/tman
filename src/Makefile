CFLAGS := -ansi -Wall -Werror

all: clean build

debug: CFLAGS += -D __DEBUG
debug: clean build

build: sidekick libtman.so tman

libtman.so:
	$(CC) $(CFLAGS) --shared -ldl -lrt -fPIC -o $@ -c $<

tman: tman.o
	$(CC) $(CFLAGS) -lrt -lm -o $@ $<

tman.o: tman.c
	$(CC) $(CFLAGS) -o $@ -c $<

sidekick: sidekick.o
	$(CC) $(CFLAGS) -o $@ $<

sidekick.o: sidekick.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	$(RM) ./libtman.so
	$(RM) ./*.o
	$(RM) ./tman
	$(RM) ./sidekick

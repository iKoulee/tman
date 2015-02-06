all: clean libtman.so tman

libtman.so:
	$(CC) -ansi -Wall --shared -ldl -lrt -fPIC -o $@ libtman.c

tman:
	$(CC) -ansi -Wall -lrt -lm -o $@ tman.c

clean:
	$(RM) ./libtman.so
	$(RM) ./tman

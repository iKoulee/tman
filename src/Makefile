all: libtman.so tman

libtman.so:
	$(CC) -ansi -Wall --shared -ldl -fPIC -o $@ libtman.c

tman:
	$(CC) -ansi -Wall -o $@ tman.c

clean:
	rm ./libtman.so
	rm ./tman

#define _GNU_SOURCE

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SLEEP_INTERVAL 1

int main(int argc, char **argv) {
    struct timespec time;
    long int count, i;

    if (argc < 2) {
        return 1;
    }

    count = strtol(argv[1], NULL, 0);
    for (i = 0; i < count; i++) {
        printf("Return code: %d\n", clock_gettime(CLOCK_REALTIME, &time));
        printf("Second since epoch: %ld\n", time.tv_sec);
        printf("Current local time and date: %s\n", asctime(localtime(&time.tv_sec)));
        if ( 0 != sleep(SLEEP_INTERVAL) )
            break;
    }
    return 0;
}

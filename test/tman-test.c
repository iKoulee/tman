#define _GNU_SOURCE

#include <time.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SLEEP_INTERVAL 1

int main(int argc, char **argv) {
    struct timespec ts;
    struct timeval tv;
    long int count, i;

    if (argc < 2) {
        return 1;
    }

    count = strtol(argv[1], NULL, 0);
    for (i = 0; i < count; i++) {
        printf("Calling 'clock_gettime(CLOCK_REALTIME, &time)': ");
        if (! (clock_gettime(CLOCK_REALTIME, &ts))) {
            printf("%ld\n", ts.tv_sec);
        } else {
            puts("FAIL\n");
        }
        printf("Calling 'gettimeofday(&tv,NULL)': ");
        if (! (gettimeofday(&tv,NULL))) {
            printf("%ld\n", tv.tv_sec);
        } else {
            puts("FAIL\n");
        }
        if ( 0 != sleep(SLEEP_INTERVAL) )
            break;
    }
    return 0;
}

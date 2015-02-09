/*
 ******************************************************************************
 * This file is part of Tman.
 *
 * Tman is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * Foobar is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Tman.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Copyright (C) 2014 Red Hat, Inc., Jakub Prokeš <jprokes@redhat.com>
 * Author: Jakub Prokeš <jprokes@redhat.com>
 *
 ******************************************************************************
 */

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
    time_t myTime;
    long int count, i;

    if (argc < 2) {
        return 1;
    }

    count = strtol(argv[1], NULL, 0);
    for (i = 0; i < count; i++) {
        printf("(%ld)=============================================================================\n",i);
        printf("Calling 'clock_gettime(CLOCK_REALTIME, &time)': ");
        if (! (clock_gettime(CLOCK_REALTIME, &ts))) {
            printf("%ld = %s\n", ts.tv_sec, asctime(localtime(&ts.tv_sec)));
        } else {
            puts("FAIL\n");
        }
        printf("Calling 'gettimeofday(&tv,NULL)': ");
        if (! (gettimeofday(&tv,NULL))) {
            printf("%ld = %s\n", tv.tv_sec, asctime(localtime(&tv.tv_sec)));
        } else {
            puts("FAIL\n");
        }
        printf("Calling 'time(&myTime)': ");
        if ((time(&myTime)) != -1) {
            printf("%ld = %s\n", myTime, asctime(localtime(&myTime)));
        } else {
            puts("FAIL\n");
        }

        if ( 0 != sleep(SLEEP_INTERVAL) )
            break;
    }
    return 0;
}

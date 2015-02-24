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
#include <getopt.h>
#include <unistd.h>
#include <locale.h>

#define DEFAULT_SLEEP_INTERVAL 1
#define TIME_STRING_BUFFER_SIZE 1024
#define MASK_CLOCK_GETTIME 1
#define MASK_GETTIMEOFDAY 2
#define MASK_TIME 4

static int verbose = 0;
static int humanize = 0;
static char *locale = NULL;

void printUsage(char *progName) {
    puts("Usage:");
    printf("%s <--time|--gettimeofday|--clock_gettime> [OPTIONS]\n", progName);
    puts("\t-c <count>\tNumber of repeating");
    puts("\t-d <sec>\tDelay betwen repeating");
    puts("\t-H --humanize\tShow time in human readable localized form");
    puts("\t-v --verbose\tVerbose output");
    puts("\t-h --help\tShow this help and exit");
    puts("\t--time\t\tAsk for time throug function time");
    puts("\t--gettimeofday\tAsk for time throug function gettimeofday");
    puts("\t--clock_gettime\tAsk for time throug function clock_gettime\n");
}


void printHumanized(time_t seconds) {
    char *sBuffer = malloc(TIME_STRING_BUFFER_SIZE);
    if (!sBuffer) {
        fputs("Memmory allocation failed!", stderr);
        exit(EXIT_FAILURE);
    }

    struct tm tmBuffer;

    if (!localtime_r(&seconds, &tmBuffer))
        exit(EXIT_FAILURE);

    if (! locale) {
        locale = getenv("LC_TIME");
    }
    if ( locale ) {
        if (! setlocale(LC_TIME,locale)) {
            fprintf(stderr,"Unable to set locale to '%s'\n", locale);
        }
    }

    if (!strftime(sBuffer, TIME_STRING_BUFFER_SIZE, "%c", &tmBuffer))
        exit(EXIT_FAILURE);

    printf("%s\n", sBuffer);
    free(sBuffer);
}

int main(int argc, char **argv) {
    struct timespec ts;
    struct timeval tv;
    time_t myTime;
    time_t delay = DEFAULT_SLEEP_INTERVAL;
    int count;
    int fMask = 0;
    int flag;

    if (argc < 2) {
        return 1;
    }

    while (1) {
        int indexPtr = 0;
        static char *shortOpts = "c:d:vhH";
        static struct option longOpts[] = {
            {"clock_gettime", no_argument, NULL, 1},
            {"gettimeofday", no_argument, NULL, 2},
            {"time", no_argument, NULL, 4},
            {"count", required_argument, NULL, 'c'},
            {"delay", required_argument, NULL, 'd'},
            {"verbose", no_argument, &verbose, 'v'},
            {"humanize", no_argument, &humanize, 'H'},
            {"help", no_argument, NULL, 'h'},
            {0, 0, 0, 0}
        };

        flag = getopt_long(argc, argv, shortOpts, longOpts, &indexPtr);

        if (flag == -1)
            break;

        switch (flag) {
            case MASK_CLOCK_GETTIME:
            case MASK_GETTIMEOFDAY:
            case MASK_TIME:
                fMask = fMask | flag;
                break;
            case 'c':
                count = (int) strtol(optarg, NULL, 0);
                break;
            case 'd':
                delay = strtol(optarg,NULL, 0);
                break;
            case 'h':
                printUsage(argv[0]);
                exit(EXIT_SUCCESS);
            case 'H':
                humanize = 1;
                break;
            case 'v':
                verbose = 1;
                break;
        }
    }

    while (1) {
        if (fMask & MASK_CLOCK_GETTIME) {
            if ( -1 != clock_gettime(CLOCK_REALTIME, &ts)) {
                if (verbose) {
                    fputs("clock_gettime(CLOCK_REALTIME...)  = ", stdout);
                }
                if (humanize) {
                    printHumanized(ts.tv_sec);
                } else {
                    printf("%ld.%ld\n", ts.tv_sec, ts.tv_nsec);
                }
            }
        }

        if (fMask & MASK_GETTIMEOFDAY) {
            if ( -1 != gettimeofday(&tv, NULL)) {
                if (verbose) {
                    fputs("gettimeofday(...)  = ", stdout);
                }
                if (humanize) {
                    printHumanized(tv.tv_sec);
                } else {
                    printf("%ld.%ld\n", tv.tv_sec, tv.tv_usec);
                }
            } else {
                fputs("Calling 'gettimeofday' failed!", stderr);
            }
        }

        if (fMask & MASK_TIME) {
            if ( -1 != time(&myTime)) {
                if (verbose) {
                    fputs("time(...)  = ", stdout);
                }
                if (humanize) {
                    printHumanized(myTime);
                } else {
                    printf("%ld\n", myTime);
                }
            } else {
                fputs("Calling 'time' failed!\n", stderr);
            }
        }

        if (count >  0) {
            count--;
            if (! count)
                break;
        }

        if (sleep(delay)) {
            fputs("Sleep interupted!", stderr);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

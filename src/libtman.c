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

#define _GNU_SOURCE             /* required to get RTLD_NEXT defined */
#define __USE_GNU               /* enable conversions timeval <-> timesec */

#include <stdio.h>
/*#include <time.h>*/
#include <dlfcn.h>
#include <stdlib.h>
#include <errno.h>
#include <error.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <mqueue.h>
#include "tman.h"

typedef struct _lNode {
    tmanMSG_t * shift;
    struct _lNode * next;
} qNode;

struct msgList {
    qNode * begin;
    qNode * current;
};

/* sys/time.h */
static int (*orig_gettimeofday)(struct timeval *, struct timezone *);

/* time.h */
static int (*orig_clock_gettime)(clockid_t, struct timespec *);
static time_t (*orig_time)(time_t *res);

mqd_t mQ;
struct timespec execTime;
struct msgList mList;

#define print_debug(ts) printf("Call: '%s' %ld,%ld\n", __func__, (ts)->tv_sec, (ts)->tv_nsec);


void __attribute__ ((constructor)) libtman_init(void) {
    struct mq_attr mqAttr;
    pid_t myPID;
    char mqName[MQ_MAXNAMELENGTH];

    orig_gettimeofday   = dlsym(RTLD_NEXT, "gettimeofday");
    orig_clock_gettime  = dlsym(RTLD_NEXT, "clock_gettime");
    orig_time           = dlsym(RTLD_NEXT, "time");

    if (orig_clock_gettime == NULL)
        exit(EXIT_FAILURE);
    if ((*orig_clock_gettime)(CLOCK_MONOTONIC, &execTime)) {
        fprintf(stderr,"Cannot get current time.");
        exit(EXIT_FAILURE);
    }

/*
 *  Create message queue
 */

    mqAttr.mq_msgsize = MQ_MSGSIZE;
    mqAttr.mq_maxmsg = MQ_MAXMSG;

    myPID = getpid();
    snprintf(mqName, MQ_MAXNAMELENGTH, "%s.%d", MQ_PREFIX, myPID);
    mQ = mq_open(mqName, O_CREAT | O_RDONLY | O_NONBLOCK, 0600, &mqAttr);
    if (mQ == -1) {
        switch (errno) {
            case EACCES:
                perror("Cannot get a message queue: Insuficient rights.\n");
            break;
            case EEXIST:
                perror("Cannot create message queue: Queue exist\n");
            break;
            case EINVAL:
                perror("Cannot get a message queue: Incorrect attributes.\n");
            break;
            case ENOSPC:
            case ENFILE:
            case EMFILE:
                perror("Cannot create message queue: Maximal count of queues reached.\n");
            break;
            case ENAMETOOLONG:
                perror("Cannot get a message queue: Name is too long\n");
            break;
            case ENOENT:
                perror("Cannot get a message queue: Wrong queue name\n");
            break;
            case ENOMEM:
                perror("Cannot create message queue: Insufficient memory\n");
            break;
            default:
                perror("Cannot get a message queue: Unknown error.\n");
        }
    }

    printf("Library: My pid is: %d\n\n", getpid());
}

int put2Q(struct msgList *mList, tmanMSG_t *msg) {
    qNode *new, *this;
    int counter = 0;

    if (!(new = malloc(sizeof(qNode)))) {
        errno = ENOMEM;
        return -1;
    }
    memset(new, 0, sizeof(qNode));
    new->shift = msg;

    if (!mList->begin) {
        mList->begin = new;
        mList->current = mList->begin;
    } else {
        this = mList->begin;
        while (this->next) {
            if (this->next->shift->member.delay.tv_sec >= msg->member.delay.tv_sec) {
                break;
            }
            this = this->next;
            counter++;
        }
        if ((mList->begin == this) && (this->shift->member.delay.tv_sec >= msg->member.delay.tv_sec)) {
            if (mList->current == mList->begin)
                mList->current = new;
            mList->begin = new;
            new->next = this;
        } else {
            new->next = this->next;
            this->next = new;
        }
    }
    return counter;
}

int checkMessageInQueue( mqd_t mQ) {
    ssize_t bytesRead;
    tmanMSG_t *msg;
    int counter = 0;

    if (! (msg = malloc(MQ_MSGSIZE))) {
        errno = ENOMEM;
        return -1;
    }

    memset(msg->data, 0, MQ_MSGSIZE);
    bytesRead = mq_receive(mQ, msg->data, MQ_MSGSIZE, NULL);
    while (bytesRead != -1) {
        printf("Library: Got MSG:\n - Delay: %ld\n - Type: %d\n - Delta: %ld\n",
                msg->member.delay.tv_sec, msg->member.type, msg->member.delta.tv_sec);
        printf("Library: Record inserted on the %dth position\n", put2Q(&mList, msg));
        if (!(msg = malloc(MQ_MSGSIZE))) {
            errno = ENOMEM;
            return -1;
        }
        memset(msg->data, 0, MQ_MSGSIZE);
        bytesRead = mq_receive(mQ, msg->data, MQ_MSGSIZE, NULL);
    }
    if ((bytesRead == -1) && (errno != EAGAIN))
        return -1;
    free(msg);
    return counter;
}

int timeControll(struct timespec *ts) {
    struct timespec running;

    if ((*orig_clock_gettime)(CLOCK_MONOTONIC, &running)) {
        return -1;
    }

    running.tv_sec = running.tv_sec - execTime.tv_sec;

    if (checkMessageInQueue(mQ) == -1) {
        fprintf(stderr, "Error %d: %s\n", errno, strerror(errno));
        return -1;
    }

    if (!mList.begin) { /* there is no guidelines to change time */
        puts("Qeue is empty");
        return 0;
    }

    if (!mList.current) { /* better safe than sorry */
        mList.current = mList.begin;
    }
    
    if (mList.current->next) { /* if is time for next record than change it */
/*        printf("Library: Next change in %ld second(s).\n",
                mList.current->next->shift->member.delay.tv_sec - running.tv_sec);
*/
        while ( mList.current->next->shift->member.delay.tv_sec < running.tv_sec ) {
            mList.current = mList.current->next;
            if (!mList.current->next)
                break;
        }
    }

    if (mList.current->shift->member.delay.tv_sec <= running.tv_sec) {
        mList.current->shift->member.delta.tv_sec, mList.current->shift->member.delay.tv_sec);

        switch ( mList.current->shift->member.type ) {
            case T_SET:
                ts->tv_sec = mList.current->shift->member.delta.tv_sec;
                ts->tv_nsec = mList.current->shift->member.delta.tv_nsec;
                break;
            case T_SUB:
                ts->tv_sec -=  mList.current->shift->member.delta.tv_sec;
                ts->tv_nsec -= mList.current->shift->member.delta.tv_nsec;
                printf("%ld (-) %ld\n", ts->tv_sec, mList.current->shift->member.delta.tv_sec);
                break;
            case T_ADD:
                ts->tv_sec +=  mList.current->shift->member.delta.tv_sec;
                ts->tv_nsec += mList.current->shift->member.delta.tv_nsec;
                printf("%ld (+) %ld\n", ts->tv_sec, mList.current->shift->member.delta.tv_sec);
                break;
            case T_MUL:
                ts->tv_sec = (running.tv_sec * mList.current->shift->member.delta.tv_sec) + ts->tv_sec;
                break;
        }
    }
    return 0;
}

/*
 * Wraped functions
 */

int clock_gettime(clockid_t clk_id, struct timespec *tp) {
    if (!orig_clock_gettime) {
        return -1;
    }
    if ((*orig_clock_gettime)(clk_id, tp)) {
        return -1;
    }
    return timeControll(tp);
}

int gettimeofday(struct timeval *tv, struct timezone *tz) {
    struct timespec ts;
    if (!orig_gettimeofday) {
        return 1;
    }
    if ((*orig_gettimeofday)(tv, tz)) {
        return -1;
    }
    TIMEVAL_TO_TIMESPEC(tv, &ts);
    if (timeControll(&ts)) {
        return -1;
    }
    TIMESPEC_TO_TIMEVAL(tv, &ts);
    return 0;
}

time_t time(time_t *res) {
    time_t myTime;
    struct timespec ts;
    if (!orig_time) {
        return -1;
    }
    if ((myTime = (*orig_time)(res)) == -1) {
        return -1;
    }
    ts.tv_sec = myTime;
    if (timeControll(&ts)) {
        return -1;
    }
    if (res) {
        *res = ts.tv_sec;
    }
    return (time_t)ts.tv_sec;
}


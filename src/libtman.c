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

mqd_t mQ;
struct timespec execTime;
struct msgList mList;

#define print_debug(ts) printf("Call: '%s' %ld,%ld\n", __func__, (ts)->tv_sec, (ts)->tv_nsec);


void __attribute__ ((constructor)) libtman_init(void) {
    struct mq_attr mqAttr;
    pid_t myPID;
    char mqName[MQ_MAXNAMELENGTH];

    /*
    mList.begin = NULL;
    mList.current = NULL;
    */

    orig_gettimeofday   = dlsym(RTLD_NEXT, "gettimeofday");
    orig_clock_gettime  = dlsym(RTLD_NEXT, "clock_gettime");

    if (orig_clock_gettime == NULL)
        exit(1);
    if ((*orig_clock_gettime)(CLOCK_MONOTONIC, &execTime)) {
        printf("clock_gettime failed ... leaving\n");
        exit(1);
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

    if (mList->begin == NULL) {
        mList->begin = new;
        mList->current = mList->begin;
    } else {
        this = mList->begin;
        while (this->shift->member.delay.tv_sec < msg->member.delay.tv_sec) {
            counter++;
            if (this->next == NULL)
                break;
            this = this->next;
        }
        new->next = this->next;
        this->next = new;
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
        printf("Library: Got MSG:\n - Delay: %ld\n - Type: %d\n - Delta: %ld\n", msg->member.delay.tv_sec,
                msg->member.type, msg->member.delta.tv_sec);
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
        printf("clock_gettime failed ... leaving\n");
        exit(1);
    }

    running.tv_sec = running.tv_sec - execTime.tv_sec;
    printf("Library: Running time: %ld\n", running.tv_sec);
    if (checkMessageInQueue(mQ) == -1) {
        puts(strerror(errno));
    }

    if (!mList.begin) { /* there is no guidelines to change time */
        puts("Library: empty list");
        return 0;
    }

    if (!mList.current) { /* better safe than sorry */
        mList.current = mList.begin;
    }
    
    if (mList.current->next) { /* if is time for next record than change it */
        printf("Library: Next change in %ld second(s).\n",
                mList.current->next->shift->member.delay.tv_sec - running.tv_sec);
        while ( mList.current->next->shift->member.delay.tv_sec < running.tv_sec ) {
            mList.current = mList.current->next;
            if (!mList.current->next)
                break;
        }
    }
    printf("Library: Found time shift: %ld after %ld\n",
            mList.current->shift->member.delta.tv_sec, mList.current->shift->member.delay.tv_sec);

    switch ( mList.current->shift->member.type ) {
        case T_SET:
            ts->tv_sec = mList.current->shift->member.delta.tv_sec;
            ts->tv_nsec = mList.current->shift->member.delta.tv_nsec;
            break;
        case T_SUB:
            ts->tv_sec -=  mList.current->shift->member.delta.tv_sec;
            ts->tv_nsec -= mList.current->shift->member.delta.tv_nsec;
            break;
        case T_ADD:
            ts->tv_sec +=  mList.current->shift->member.delta.tv_sec;
            ts->tv_nsec += mList.current->shift->member.delta.tv_nsec;
            break;
        case T_MUL:
            ts->tv_sec = (running.tv_sec * mList.current->shift->member.delta.tv_sec) + ts->tv_sec;
            break;
    }
    return 0;
}

int clock_gettime (clockid_t clk_id, struct timespec *tp) {
    puts("Library: Handling clock_gettime");
    if (orig_clock_gettime == NULL) {
        puts("Function: 'clock_gettime' not found\n");
        return -1;
    }
    if ((*orig_clock_gettime)(clk_id, tp)) {
        printf("clock_gettime failed ... leaving\n");
        return -1;
    }
    return timeControll(tp);
}

int gettimeofday (struct timeval *tv, struct timezone *tz) {
    printf("Call gettimeofday\n");
    return (*orig_gettimeofday)(tv, tz);
}

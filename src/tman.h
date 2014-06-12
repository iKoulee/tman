
#ifndef _TMAN_H
#define _TMAN_H

#include <time.h>

typedef union {
    struct _Member {
        struct timespec delay;
        unsigned char type;
        struct timespec delta;
    } member;
    char data[sizeof(struct _Member)];
} tmanMSG_t;

#define T_SET 1
#define T_SUB 2
#define T_ADD 4
#define T_MUL 8

#define MQ_MSGSIZE sizeof(tmanMSG_t)
#define MQ_MAXMSG 5
#define MQ_PREFIX "/tman"
#define MQ_MAXNAMELENGTH 64


#endif

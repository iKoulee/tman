
#ifndef _TMAN_H

struct timeShift {
    int test;
};


struct timeMsgBuf {
    long mtype; /* must be positive number */
    struct timeShift mdata;
};

#endif

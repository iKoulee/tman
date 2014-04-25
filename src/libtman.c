#define _GNU_SOURCE             /* required to get RTLD_NEXT defined */
#define __USE_GNU               /* enable conversions timeval <-> timesec */

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <dlfcn.h>

/* sys/time.h */
static int (*orig_gettimeofday)(struct timeval *, struct timezone *);

/* time.h */
static int (*orig_clock_gettime)(clockid_t, struct timespec *);


#define print_debug(ts) printf("Call: '%s' %ld,%ld\n", __func__, (*(ts)).tv_sec, (*(ts)).tv_nsec);

void __attribute__ ((constructor)) libtman_init(void) {
    orig_gettimeofday   = dlsym(RTLD_NEXT, "gettimeofday");
    orig_clock_gettime  = dlsym(RTLD_NEXT, "clock_gettime");
}

static int time_control(struct timespec *ts) {
    (*ts).tv_sec -= 1800;
    return 0;
}

int clock_gettime (clockid_t clk_id, struct timespec *tp) {
    if (orig_clock_gettime == NULL)
        return -1;
    if ((*orig_clock_gettime)(clk_id, tp)) {
        printf("clock_gettime failed ... leaving\n");
        return -1;
    }
    print_debug(tp);
    return time_control(tp);
}

int gettimeofday (struct timeval *tv, struct timezone *tz) {
    printf("Call gettimeofday\n");
    return (*orig_gettimeofday)(tv, tz);
}

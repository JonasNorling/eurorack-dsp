#pragma once
#include <zephyr/sys/clock.h>
#include <time.h>

typedef struct 
{
    int calls;
    long long total_ns;
    long long worst_ns;
    struct timespec start_ts;
} perftimer_t;

static inline void perftimer_start(perftimer_t *pt)
{
    (void)sys_clock_gettime(SYS_CLOCK_MONOTONIC, &pt->start_ts);
}

static inline void perftimer_end(perftimer_t *pt)
{
    struct timespec ts;
    (void)sys_clock_gettime(SYS_CLOCK_MONOTONIC, &ts);
    const long long unsigned ns =
        1000000000LL * (ts.tv_sec - pt->start_ts.tv_sec) +
        (ts.tv_nsec - pt->start_ts.tv_nsec);
    pt->total_ns += ns;
    if (ns > pt->worst_ns) {
        pt->worst_ns = ns;
    }
    pt->calls++;
}

static inline void perftimer_report(const char *name, perftimer_t pt)
{
    LOG_INF("Timer %s: %d calls, avg %llu ns, worst %llu ns",
        name, pt.calls, pt.total_ns / pt.calls, pt.worst_ns);
}

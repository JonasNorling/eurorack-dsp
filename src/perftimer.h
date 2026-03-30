#pragma once
#include <zephyr/sys/clock.h>
#include <time.h>

typedef struct 
{
    int calls;
    uint64_t running_cyc;
    uint64_t off_cyc;
    uint64_t worst_cyc;
    uint64_t start_cyc;
    uint64_t end_cyc;
} perftimer_t;

static inline void perftimer_start(perftimer_t *pt)
{
    const uint64_t now = k_cycle_get_64();
    pt->start_cyc = now;
    if (pt->end_cyc != 0) {
        pt->off_cyc += now - pt->end_cyc;
    }
}

static inline void perftimer_end(perftimer_t *pt)
{
    // FIXME: Handle overflow
    const uint64_t now = k_cycle_get_64();
    pt->end_cyc = now;
    const uint64_t cyc = now - pt->start_cyc;
    pt->running_cyc += cyc;
    if (cyc > pt->worst_cyc) {
        pt->worst_cyc = cyc;
    }
    pt->calls++;
}

static inline void perftimer_report(const char *name, perftimer_t pt)
{
    int duty_pct = (100 * pt.running_cyc) / pt.off_cyc;
    LOG_INF("Timer %s: %d calls, avg %llu cycles, worst %llu cycles, duty: %d%%",
        name, pt.calls, pt.running_cyc / pt.calls, pt.worst_cyc, duty_pct);
}

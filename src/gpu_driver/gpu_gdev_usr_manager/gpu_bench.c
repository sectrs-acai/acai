/*
 * benchmark primitives to benchmark x86 fraction of experiment
 */
#include "gpu_bench.h"
#include "tsc_x86.h"
#include <stdio.h>

static bench_time_t bench_times[_BENCH_SIZE_];
static unsigned long bench_count[_BENCH_SIZE_];

inline int util__validate_event(int event)
{
    if (event < 0 || event >= _BENCH_SIZE_) {
        printf("ERROR: event: %d is outside of bench size: %d\n", event, _BENCH_SIZE_);
        return -1;
    }
    return 0;
}

void bench__init(void)
{
    init_tsc();
    for (int i = 0; i < _BENCH_SIZE_; i++) {
        bench_times[i] = 0;
        bench_count[i] = 0;
    }
}

inline bench_time_t bench__start(void)
{
    return start_tsc();
}

inline void bench__stop(bench_time_t start, int event)
{
    bench_time_t time = stop_tsc(start);

    if (__glibc_unlikely(util__validate_event(event) < 0)) {
        return;
    }
    bench_times[event] += time;
    bench_count[event] += 1;
}

void bench__results(FILE *fp)
{
    #define PRINT(fmt, ...) \
    fprintf(stderr, fmt, ##__VA_ARGS__); \
    if (fp != NULL) {fprintf(fp, fmt, ##__VA_ARGS__);}
    myInt64 total = 0;

    PRINT("# T110 Times\n");
    PRINT("# T110; type; name; number-of-calls; timing (rdtsc ticks)\n");
    for (int i = 0; i < _BENCH_SIZE_; i++) {
        PRINT("T110; %d; %s; %lld; %lld\n", i, bench_str(i), bench_count[i], bench_times[i]);
        total += bench_times[i];

        // sanity check
        if (bench_times[i] > 0 && bench_count[i] == 0
            || bench_count[i] > 0 && bench_times[i] == 0) {
            PRINT("WARN: Inconsistent state, timing[%s] = %lld, count = %lld\n",
                  bench_str(i), bench_times[i], bench_count[i]);
        }
    }
    PRINT("T111; %s; %lld\n", "total rdtsc ticks", total);
}

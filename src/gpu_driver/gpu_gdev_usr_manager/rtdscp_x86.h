#ifndef RTDSCP_X86_H
#define RTDSCP_X86_H

#include <x86intrin.h>
#include <stdint.h>
#define VOLATILE __volatile__
#define ASM __asm__

#define bench_time_t unsigned long long
#define INT32 unsigned int

inline static void init_tsc()
{
    // nothing on x86
}

inline static bench_time_t start_tsc(void)
{
    unsigned int dummy;
    return __rdtscp(&dummy);
}

inline static bench_time_t stop_tsc(bench_time_t start)
{
    unsigned int dummy;
    bench_time_t stop = __rdtscp(&dummy);
    return stop - start;
}
#endif
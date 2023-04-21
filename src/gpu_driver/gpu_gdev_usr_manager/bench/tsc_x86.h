// intel manual https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/ia-32-ia-64-benchmark-code-execution-paper.pdf

#ifndef TSC_X86_H
#define TSC_X86_H

#define VOLATILE __volatile__
#define ASM __asm__

#define myInt64 unsigned long long
#define INT32 unsigned int

#define COUNTER_LO(a) ((a).int32.lo)
#define COUNTER_HI(a) ((a).int32.hi)
#define COUNTER_VAL(a) ((a).int64)

#define COUNTER(a)                                \
    ((unsigned long long)COUNTER_VAL(a))

#define COUNTER_DIFF(a, b)                        \
    (COUNTER(a)-COUNTER(b))

typedef union {
    myInt64 int64;
    struct { INT32 lo, hi; } int32;
} tsc_counter;

#define RDTSC(cpu_c)                                                    \
    ASM VOLATILE ("rdtsc" : "=a" ((cpu_c).int32.lo), "=d"((cpu_c).int32.hi))
#define CPUID()                                                \
    ASM VOLATILE ("cpuid" : : "a" (0) : "bx", "cx", "dx" )

inline static void init_tsc()
{
    // nothing on x86
}

inline static myInt64 start_tsc(void)
{
    tsc_counter start;
    CPUID(); /* flush instruction pipeline to prevent oo exec */
    RDTSC(start);
    return COUNTER_VAL(start);
}

inline static myInt64 stop_tsc(myInt64 start)
{
    tsc_counter end;
    RDTSC(end);
    CPUID();
    return COUNTER_VAL(end) - start;
}

#endif
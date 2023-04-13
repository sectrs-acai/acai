#ifndef CCA_BENCHMARK_H_
#define CCA_BENCHMARK_H_

#if defined(__x86_64__) || defined(_M_X64)
#define CCA_MARKER(marker)
#else
#define STR(s) #s
#define CCA_MARKER(marker) __asm__ volatile("MOV XZR, " STR(marker))
/*/#define CCA_MARKER(marker) __asm__ volatile("MOV XZR, %0" ::"i"(marker) :) */
#endif


// -----------------------------------------------------------------------
/* for linux: */

#define CCA_MARKER_BENCH_START CCA_MARKER(0x1)
#define CCA_MARKER_BENCH_STOP CCA_MARKER(0x2)

#define CCA_MARKER_RODINIA_INIT CCA_MARKER(0x1000)
#define CCA_MARKER_RODINIA_MEMALLOC CCA_MARKER(0x1001)
#define CCA_MARKER_RODINIA_H_TO_D CCA_MARKER(0x1002)
#define CCA_MARKER_RODINIA_EXEC CCA_MARKER(0x1003)
#define CCA_MARKER_RODINIA_D_TO_H CCA_MARKER(0x1004)
#define CCA_MARKER_RODINIA_CLOSE CCA_MARKER(0x1005)

#define CCA_BENCHMARK_START                                                    \
  CCA_TRACE_START; \
  CCA_MARKER_BENCH_START

#define CCA_BENCHMARK_STOP                                                     \
  CCA_MARKER_BENCH_STOP; \
  CCA_TRACE_STOP


#if defined(__x86_64__) || defined(_M_X64)
#define CCA_BENCHMARK_INIT
#define CCA_TRACE_START
#define CCA_TRACE_STOP

#else
#define CCA_TRACE_START __asm__ volatile("HLT 0x1337");
#define CCA_TRACE_STOP __asm__ volatile("HLT 0x1337");

// aarch64
#include <signal.h>
#include <ucontext.h>

static void __cca_sighandler(int signo, siginfo_t *si, void *data) {
  ucontext_t *uc = (ucontext_t *)data;
  uc->uc_mcontext.pc += 4;
}

#define CCA_BENCHMARK_INIT                                                     \
  {                                                                            \
    struct sigaction sa, osa;                                                  \
    sa.sa_flags = SA_ONSTACK | SA_RESTART | SA_SIGINFO;                        \
    sa.sa_sigaction = __cca_sighandler;                                        \
    sigaction(SIGILL, &sa, &osa);                                              \
  }
#endif

#endif // CCA_BENCHMARK_H_

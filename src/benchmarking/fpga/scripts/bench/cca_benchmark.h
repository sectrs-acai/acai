#ifndef CCA_BENCHMARK_H_
#define CCA_BENCHMARK_H_

#if defined(__x86_64__) || defined(_M_X64)
#define CCA_MARKER(marker)
#else
#define STR(s) #s
#define CCA_MARKER(marker) __asm__ volatile("MOV XZR, " STR(marker))
#endif

// -----------------------------------------------------------------------

#ifdef CCA_NO_BENCH
#define IS_CCA_NO_BENCH 1
#define CCA_BENCHMARK_START
#define CCA_BENCHMARK_STOP
#else
#define IS_CCA_NO_BENCH 0
#define CCA_BENCHMARK_START                                                    \
  CCA_TRACE_START;                                                             \
  CCA_FLUSH;                                                                   \
  CCA_MARKER(0x1)

#define CCA_BENCHMARK_STOP                                                     \
  CCA_MARKER(0x2);                                                             \
  CCA_FLUSH;                                                                   \
  CCA_TRACE_STOP
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define CCA_FLUSH
#define CCA_BENCHMARK_INIT
#define CCA_TRACE_START
#define CCA_TRACE_STOP

#else
#define CCA_FLUSH __asm__ volatile("ISB");
#define CCA_TRACE_START __asm__ volatile("HLT 0x1337");
#define CCA_TRACE_STOP __asm__ volatile("HLT 0x1337");

// aarch64
#include <signal.h>
#include <ucontext.h>

static void __cca_sighandler(int signo, siginfo_t *si, void *data) {
  ucontext_t *uc = (ucontext_t *)data;
  uc->uc_mcontext.pc += 4;
}

#if defined(__x86_64__) || defined(_M_X64)
#define CCA_BENCHMARK_INIT
#else
#define CCA_BENCHMARK_INIT                                                     \
  {                                                                            \
    struct sigaction sa, osa;                                                  \
    sa.sa_flags = SA_ONSTACK | SA_RESTART | SA_SIGINFO;                        \
    sa.sa_sigaction = __cca_sighandler;                                        \
    sigaction(SIGILL, &sa, &osa);                                              \
  }
#endif
#endif

#define CCA_FPGA_WARMUP_ITERATION CCA_MARKER(0x160)
#define CCA_FPGA_BENCH_ITERATION CCA_MARKER(0x161)

#define CCA_FPGA_INIT_START CCA_MARKER(0x1050)
#define CCA_FPGA_INIT_STOP CCA_MARKER(0x1051)

#define CCA_FPGA_H_TO_D_START CCA_MARKER(0x1052)
#define CCA_FPGA_H_TO_D_STOP CCA_MARKER(0x1053)

#define CCA_FPGA_D_TO_H_START CCA_MARKER(0x1054)
#define CCA_FPGA_D_TO_H_STOP CCA_MARKER(0x1055)

#define CCA_FPGA_EXEC_START CCA_MARKER(0x1056)
#define CCA_FPGA_EXEC_STOP CCA_MARKER(0x1057)

#endif // CCA_BENCHMARK_H_

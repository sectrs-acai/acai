#ifndef CCA_BENCHMARK_H_
#define CCA_BENCHMARK_H_

/*
 *  Marker instruction:
 *  Non exception raising asm: add marker to xzr register which is nop
 */
#define STR(s) #s
#define CCA_BENCHMARK_MARKER(marker) __asm__ volatile("MOV XZR, " STR(marker))


/*
 * Issues: linux will emit an illegal instruction signal
 * when encountering HLT instruction.
 * However: HLT plugin is much faster than always tracing all executed
 * instructions. Workaround: aarch64 instruction size always 4 bytes, skip
 * illegal instruction and use hlt to start/stop measurements
 */
#define CCA_START_TRACING __asm__ volatile("HLT 0x1337");
#define CCA_STOP_TRACING __asm__ volatile ("HLT 0x1337");

#define CCA_START_BENCHMARK                                             \
  CCA_START_TRACING;                                                           \
  CCA_BENCHMARK_MARKER(0x1337)

#define CCA_STOP_BENCHMARK                                              \
  CCA_BENCHMARK_MARKER(0x1338);                                             \
  CCA_STOP_TRACING

#include <signal.h>
#include <ucontext.h>
static void __cca_sighandler(int signo, siginfo_t si, void *data) {
  ucontext_t *uc = (ucontext_t *)data;
  uc->uc_mcontext.pc += 4;
}

#define CCA_BENCHMARK_INIT                    \
 { \
  struct sigaction sa, osa; \
  sa.sa_flags = SA_ONSTACK | SA_RESTART | SA_SIGINFO; \
  sa.sa_sigaction = __cca_sighandler; \
  sigaction(SIGILL, &sa, &osa); \
 }


#endif // CCA_BENCHMARK_H_

#define GNU_SOURCE
#include <stdio.h>
#include "cca_benchmark.h"

int main() {
  CCA_BENCHMARK_INIT;

  CCA_START_BENCHMARK;
  CCA_BENCHMARK_MARKER(0x234);
  CCA_BENCHMARK_MARKER(0x1);
  CCA_BENCHMARK_MARKER(0x1);
  CCA_BENCHMARK_MARKER(0x1);
  CCA_BENCHMARK_MARKER(0x1);
  printf("done\n");
  CCA_STOP_BENCHMARK;

  return 0;
}

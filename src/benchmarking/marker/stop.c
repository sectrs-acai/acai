#define GNU_SOURCE
#include <stdio.h>
#include "cca_benchmark.h"

int main() {
  CCA_BENCHMARK_INIT;
  CCA_BENCHMARK_STOP;

  printf("stop tracing\n");

  return 0;
}

#define GNU_SOURCE
#include <stdio.h>
#include "cca_benchmark.h"

int main() {
  CCA_BENCHMARK_INIT;

  CCA_BENCHMARK_START;
  CCA_MARKER(0x100);
  CCA_MARKER(0x110);
  CCA_MARKER(0x120);
  CCA_MARKER(0x130);

  CCA_MARKER_RODINIA_INIT;
  for(int i = 0; i < 10; i++) {
  printf("done\n");
  }

  CCA_MARKER_RODINIA_CLOSE;

  CCA_BENCHMARK_STOP;

  return 0;
}

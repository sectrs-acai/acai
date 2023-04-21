#ifndef GPU_BENCH_H_
#define GPU_BENCH_H_

#include <stdio.h>

#define BENCH_FPGA_OPEN 0x1
#define BENCH_FPGA_CLOSE 0x2
#define BENCH_FPGA_MMAP 0x3
#define BENCH_FPGA_UNMMAP 0x4
#define BENCH_FPGA_DMA 0x5

#define _BENCH_SIZE_ 0x6


#define bench_time_t unsigned long long

static const char *bench_str(int type)
{
    switch (type) {
        case BENCH_FPGA_OPEN: return "BENCH_FPGA_OPEN";
        case BENCH_FPGA_CLOSE: return "BENCH_FPGA_CLOSE";
        case BENCH_FPGA_MMAP: return "BENCH_FPGA_MMAP";
        case BENCH_FPGA_UNMMAP: return "BENCH_FPGA_UNMMAP";
        case BENCH_FPGA_DMA: return "BENCH_FPGA_DMA";
        default:return "unknown";
    }
}

void bench__init(void);
bench_time_t bench__start();
void bench__stop(bench_time_t start, int type);
void bench__results(FILE *fp);

#endif

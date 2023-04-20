#ifndef GPU_BENCH_H_
#define GPU_BENCH_H_

#include <stdio.h>

#define BENCH_DEVICE_OPEN 0x1
#define BENCH_DEVICE_CLOSE 0x2
#define BENCH_DEVICE_IOCTL_QUERY 0x3
#define BENCH_DEVICE_IOCTL_MEMALLOC 0x4
#define BENCH_DEVICE_IOCTL_FREE 0x5
#define BENCH_DEVICE_IOCTL_VIRTGET 0x6
#define BENCH_DEVICE_IOCTL_SYNC 0x7
#define BENCH_DEVICE_IOCTL_TUNE 0x8
#define BENCH_DEVICE_IOCTL_HOST_TO_DEV 0x9
#define BENCH_DEVICE_IOCTL_DEV_TO_HOST 0xA
#define BENCH_DEVICE_IOCTL_LAUNCH 0xB
#define BENCH_DEVICE_IOCTL_BARRIER 0xC
#define _BENCH_SIZE_ 0xF


#define bench_time_t unsigned long long

static const char *bench_str(int type)
{
    switch (type) {
        case BENCH_DEVICE_OPEN: return "BENCH_DEVICE_OPEN";
        case BENCH_DEVICE_CLOSE: return "BENCH_DEVICE_CLOSE";
        case BENCH_DEVICE_IOCTL_QUERY: return "BENCH_DEVICE_IOCTL_QUERY";
        case BENCH_DEVICE_IOCTL_MEMALLOC: return "BENCH_DEVICE_IOCTL_MEMALLOC";
        case BENCH_DEVICE_IOCTL_FREE: return "BENCH_DEVICE_IOCTL_FREE";
        case BENCH_DEVICE_IOCTL_VIRTGET: return "BENCH_DEVICE_IOCTL_VIRTGET";
        case BENCH_DEVICE_IOCTL_SYNC: return "BENCH_DEVICE_IOCTL_SYNC";
        case BENCH_DEVICE_IOCTL_TUNE: return "BENCH_DEVICE_IOCTL_TUNE";
        case BENCH_DEVICE_IOCTL_HOST_TO_DEV: return "BENCH_DEVICE_IOCTL_HOST_TO_DEV";
        case BENCH_DEVICE_IOCTL_DEV_TO_HOST: return "BENCH_DEVICE_IOCTL_DEV_TO_HOST";
        case BENCH_DEVICE_IOCTL_LAUNCH: return "BENCH_DEVICE_IOCTL_LAUNCH";
        case BENCH_DEVICE_IOCTL_BARRIER: return "BENCH_DEVICE_IOCTL_BARRIER";
        default:return "unknown";
    }
}

void bench__init(void);
bench_time_t bench__start();
void bench__stop(bench_time_t start, int type);
void bench__results(FILE *fp);

#endif

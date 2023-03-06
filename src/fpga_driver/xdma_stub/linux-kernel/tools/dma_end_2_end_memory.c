#include <fcntl.h>
#include <getopt.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include "../xdma/cdev_sgdma.h"

#include "dma_utils.c"

#define DEVICE_NAME_DEFAULT "/dev/xdma0_c2h_0"
#define SIZE_DEFAULT (32)
#define COUNT_DEFAULT (1)

static int eop_flush = 0;


static int dma_from_device(
        char *devname,
        uint64_t addr,
        uint64_t aperture,
        uint64_t size,
        char **res_data,
        uint64_t offset,
        uint64_t count)
{
    ssize_t rc = 0;
    size_t out_offset = 0;
    size_t bytes_done = 0;
    uint64_t i;
    char *buffer = NULL;
    struct timespec ts_start, ts_end;
    int out_fd = - 1;
    int fpga_fd;
    long total_time = 0;
    float result;
    float avg_time = 0;
    int underflow = 0;

    /*
     * use O_TRUNC to indicate to the driver to flush the data up based on
     * EOP (end-of-packet), streaming mode only
     */
    if (eop_flush)
        fpga_fd = open(devname, O_RDWR | O_TRUNC);
    else
        fpga_fd = open(devname, O_RDWR);

    if (fpga_fd < 0)
    {
        fprintf(stderr, "unable to open device %s, %d.\n",
                devname, fpga_fd);
        perror("open device");
        return - EINVAL;
    }

    posix_memalign((void **) res_data, 4096 /*alignment */ , size + 4096);
    if (! *res_data)
    {
        fprintf(stderr, "OOM %lu.\n", size + 4096);
        rc = - ENOMEM;
        goto out;
    }

    buffer = *res_data + offset;
    if (verbose)
        fprintf(stdout, "host buffer 0x%lx, %p.\n", size + 4096, buffer);

    for (i = 0; i < count; i ++)
    {
        rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);
        if (aperture)
        {
            struct xdma_aperture_ioctl io;

            io.buffer = (unsigned long) buffer;
            io.len = size;
            io.ep_addr = addr;
            io.aperture = aperture;
            io.done = 0UL;

            rc = ioctl(fpga_fd, IOCTL_XDMA_APERTURE_R, &io);
            if (rc < 0 || io.error)
            {
                fprintf(stderr,
                        "#%d: aperture R failed %d,%d.\n",
                        i, rc, io.error);
                goto out;
            }

            bytes_done = io.done;
        } else
        {
            rc = read_to_buffer(devname, fpga_fd, buffer, size, addr);
            if (rc < 0)
                goto out;
            bytes_done = rc;

        }
        clock_gettime(CLOCK_MONOTONIC, &ts_end);

        if (bytes_done < size)
        {
            fprintf(stderr, "#%d: underflow %ld/%ld.\n",
                    i, bytes_done, size);
            underflow = 1;
        }

        /* subtract the start time from the end time */
        timespec_sub(&ts_end, &ts_start);
        total_time += ts_end.tv_nsec;
        /* a bit less accurate but side-effects are accounted for */
        if (verbose)
            fprintf(stdout,
                    "#%lu: CLOCK_MONOTONIC %ld.%09ld sec. read %ld/%ld bytes\n",
                    i, ts_end.tv_sec, ts_end.tv_nsec, bytes_done, size);
    }

    if (! underflow)
    {
        avg_time = (float) total_time / (float) count;
        result = ((float) size) * 1000 / avg_time;
        if (verbose)
            printf("** Avg time device %s, total time %ld nsec, avg_time = %f, size = %lu, BW = %f \n",
                   devname, total_time, avg_time, size, result);
        printf("%s ** Average BW = %lu, %f\n", devname, size, result);
        rc = 0;
    } else if (eop_flush)
    {
        /* allow underflow with -e option */
        rc = 0;
    } else
        rc = - EIO;

    out:
    close(fpga_fd);
    return rc;
}

static long dma_to_device(char *devname,
                          uint64_t addr,
                          uint64_t aperture,
                          uint64_t size,
                          char *data,
                          uint64_t offset,
                          uint64_t count)
{
    uint64_t i;
    ssize_t rc;
    size_t bytes_done = 0;
    size_t out_offset = 0;
    size_t in_filesize = 0;
    char *buffer = NULL;
    char *allocated = NULL;
    struct timespec ts_start, ts_end;
    int infile_fd = - 1;
    int outfile_fd = - 1;
    int fpga_fd = open(devname, O_RDWR);
    long total_time = 0;
    float result;
    float avg_time = 0;
    int underflow = 0;

    if (fpga_fd < 0)
    {
        fprintf(stderr, "unable to open device %s, %d.\n",
                devname, fpga_fd);
        perror("open device");
        return - EINVAL;
    }
    buffer = data + offset;
    if (verbose)
        fprintf(stdout, "host buffer 0x%lx = %p\n",
                size + 4096, buffer);


    for (i = 0; i < count; i ++)
    {
        /* write buffer to AXI MM address using SGDMA */
        rc = clock_gettime(CLOCK_MONOTONIC, &ts_start);

        if (aperture)
        {
            struct xdma_aperture_ioctl io;

            io.buffer = (unsigned long) buffer;
            io.len = size;
            io.ep_addr = addr;
            io.aperture = aperture;
            io.done = 0UL;
            // printf("IOCTL_XDMA_APERTURE_W\n");
            rc = ioctl(fpga_fd, IOCTL_XDMA_APERTURE_W, &io);
            if (rc < 0 || io.error)
            {
                fprintf(stdout,
                        "#%d: aperture W ioctl failed %d,%d.\n",
                        i, rc, io.error);
                goto out;
            }

            bytes_done = io.done;
        } else
        {
            // printf("write_from_buffer\n");
            HERE;
            rc = write_from_buffer(devname, fpga_fd, buffer, size,
                                   addr);
            if (rc < 0)
                goto out;

            bytes_done = rc;
        }

        rc = clock_gettime(CLOCK_MONOTONIC, &ts_end);

        if (bytes_done < size)
        {
            printf("#%d: underflow %ld/%ld.\n",
                   i, bytes_done, size);
            underflow = 1;
        }

        /* subtract the start time from the end time */
        timespec_sub(&ts_end, &ts_start);
        total_time += ts_end.tv_nsec;
        /* a bit less accurate but side-effects are accounted for */
        if (verbose)
            fprintf(stdout,
                    "#%lu: CLOCK_MONOTONIC %ld.%09ld sec. write %ld bytes\n",
                    i, ts_end.tv_sec, ts_end.tv_nsec, size);
    }

    if (! underflow)
    {
        avg_time = (float) total_time / (float) count;
        result = ((float) size) * 1000 / avg_time;
        if (verbose)
            printf("** Avg time device %s, total time %ld nsec, avg_time = %f, size = %lu, BW = %f \n",
                   devname, total_time, avg_time, size, result);
        printf("%s ** Average BW = %lu, %f\n", devname, size, result);
    }
    HERE;

    out:
    HERE;
    close(fpga_fd);
    if (rc < 0)
        return rc;
    /* treat underflow as error */
    rc = underflow ? - EIO:size;
    HERE;
    return rc;
}

#include <sys/time.h>

long long current_timestamp()
{
    struct timeval te;
    gettimeofday(&te, NULL);
    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;
    return milliseconds;
}

#include <time.h>
#include <stdlib.h>

char *alloc_data(size_t size)
{
    time_t current_time = time(NULL);
    srandom((unsigned int) current_time);
    char *mem = NULL;
    posix_memalign((void **) &mem, 4096 /*alignment */ , size + 4096);
    long int r = random();
    printf("r = %ld\n", r);
    if (mem == NULL)
    {
        fprintf(stderr, "OOM %lu.\n", size + 4096);
        exit(1);
    }
    printf("initialize %ld bytes\n", size);
    long int *m = (long int *) mem;
    for (unsigned long i = 0; i < (size / 8); i += 4096)
    {
        *(m + i) = r + i;
    }
    return mem;
}

#define MB(m) (1024L * 1024L * m)

int main(int argc, char *argv[])
{
    char filebuf[512];
    char buf[512];
    long ret = 0;
    char device_h2c[128];
    char device_c2h[128];
    int xdma_index = 0;
    size_t alloc_size_mb = MB(64);
    uint64_t address = 0;
    uint64_t aperture = 0;
    uint64_t offset = 0;
    uint64_t count = COUNT_DEFAULT;

    if (argc == 3)
    {
        xdma_index = strtol(argv[1], NULL, 10);
        alloc_size_mb = MB(strtol(argv[2], NULL, 10));
    }

    sprintf(device_h2c, "/dev/xdma0_h2c_%d", xdma_index);
    sprintf(device_c2h, "/dev/xdma0_c2h_%d", xdma_index);

    printf("using alloc size: %ld\n", alloc_size_mb);
    printf("using device h2c: %s\n", device_h2c);
    printf("using device c2h: %s\n", device_c2h);

    if (verbose)
    {
        HERE;
        fprintf(stdout,
                "dev %s, addr 0x%lx, aperture 0x%lx, size 0x%lx, offset 0x%lx, "
                "count %lu\n",
                device_h2c, address, aperture, alloc_size_mb, offset, count);
    }
    HERE;
    char *data = alloc_data(alloc_size_mb);
    printf("data allocated with random values\n");

    ret = dma_to_device(device_h2c, address, aperture, alloc_size_mb, data, offset, count);
    if (ret < 0)
    {
        printf("dma_to_device ret < 0: %ld\n", ret);
    }
    char *device_data = NULL;
    ret = dma_from_device(device_c2h,
                          address,
                          aperture,
                          alloc_size_mb,
                          &device_data,
                          offset,
                          count);
    if (ret < 0)
    {
        printf("dma_from_device ret < 0: %ld\n", ret);
    }
    int comp = 0;
    for (size_t i = 0; i < alloc_size_mb; i ++)
    {
        if (data[i] != device_data[i])
        {
            comp = 1;
            printf("difference at %d\n: %d != %d\n", i, data[i], device_data[i]);
        }
    }
    if (comp == 0)
    {
        printf("compare successful\n");
    }
    free(data);
    free(device_data);
}

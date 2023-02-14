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
        uint64_t offset,
        uint64_t count,
        char *ofname)
{
    ssize_t rc = 0;
    size_t out_offset = 0;
    size_t bytes_done = 0;
    uint64_t i;
    char *buffer = NULL;
    char *allocated = NULL;
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

    /* create file to write data to */
    if (ofname)
    {
        out_fd = open(ofname, O_RDWR | O_CREAT | O_TRUNC | O_SYNC,
                      0666);
        if (out_fd < 0)
        {
            fprintf(stderr, "unable to open output file %s, %d.\n",
                    ofname, out_fd);
            perror("open output file");
            rc = - EINVAL;
            goto out;
        }
    }

    posix_memalign((void **) &allocated, 4096 /*alignment */ , size + 4096);
    if (! allocated)
    {
        fprintf(stderr, "OOM %lu.\n", size + 4096);
        rc = - ENOMEM;
        goto out;
    }

    buffer = allocated + offset;
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

        /* file argument given? */
        if (out_fd >= 0)
        {
            rc = write_from_buffer(ofname, out_fd, buffer,
                                   bytes_done, out_offset);
            if (rc < 0 || rc < bytes_done)
                goto out;
            out_offset += bytes_done;
        }
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
    if (out_fd >= 0)
        close(out_fd);
    free(allocated);

    return rc;
}

static long dma_to_device(char *devname,
                          uint64_t addr,
                          uint64_t aperture,
                          uint64_t size,
                          int read_complete_file,
                          uint64_t offset,
                          uint64_t count,
                          char *infname)
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

    if (infname)
    {
        infile_fd = open(infname, O_RDONLY);
        if (infile_fd < 0)
        {
            fprintf(stderr, "unable to open input file %s, %d.\n",
                    infname, infile_fd);
            perror("open input file");
            rc = - EINVAL;
            goto out;
        }
        if (read_complete_file)
        {
            struct stat st;
            stat(infname, &st);
            size = st.st_size;
            printf("Complete file size of %s is: %ld bytes\n", infname, size);
        }
    }

    #if 0
    if (ofname)
    {
        outfile_fd =
                open(ofname, O_RDWR | O_CREAT | O_TRUNC | O_SYNC,
                     0666);
        if (outfile_fd < 0)
        {
            fprintf(stderr, "unable to open output file %s, %d.\n",
                    ofname, outfile_fd);
            perror("open output file");
            rc = - EINVAL;
            goto out;
        }
    }
    #endif

    posix_memalign((void **) &allocated, 4096 /*alignment */ , size + 4096);
    if (! allocated)
    {
        fprintf(stderr, "OOM %lu.\n", size + 4096);
        rc = - ENOMEM;
        goto out;
    }
    buffer = allocated + offset;
    if (verbose)
        fprintf(stdout, "host buffer 0x%lx = %p\n",
                size + 4096, buffer);

    if (infile_fd >= 0)
    {
        printf("read_to_buffer\n");
        rc = read_to_buffer(infname, infile_fd, buffer, size, 0);
        if (rc < 0 || rc < size)
            goto out;
        printf("read_to_buffer: %d\n", rc);
    }

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

        #if 0
        if (outfile_fd >= 0)
        {
            rc = write_from_buffer(ofname, outfile_fd, buffer,
                                   bytes_done, out_offset);
            if (rc < 0 || rc < bytes_done)
                goto out;
            out_offset += bytes_done;
        }
        #endif
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

    out:
    close(fpga_fd);
    if (infile_fd >= 0)
        close(infile_fd);
//    if (outfile_fd >= 0)
//        close(outfile_fd);
    free(allocated);

    if (rc < 0)
        return rc;
    /* treat underflow as error */
    rc = underflow ? - EIO:size;
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


int main(int argc, char *argv[])
{
    char filebuf[512];
    char buf[512];
    long ret = 0;
    char device_h2c[128];
    char device_c2h[128];
    int xdma_index = 0;
    uint64_t address = 0;
    uint64_t aperture = 0;
    uint64_t size = SIZE_DEFAULT;
    uint64_t offset = 0;
    uint64_t count = COUNT_DEFAULT;
    char *out_file;
    char *in_file = "/tmp/datafile_32M.bin";

    if (argc == 2)
    {
        in_file = argv[1];
    } else if (argc == 3)
    {
        in_file = argv[1];
        xdma_index = strtol(argv[2], NULL, 10);

    }

    sprintf(device_h2c, "/dev/xdma0_h2c_%d", xdma_index);
    sprintf(device_c2h, "/dev/xdma0_c2h_%d", xdma_index);

    printf("using file: %s\n", in_file);
    printf("using device h2c: %s\n", device_h2c);
    printf("using device c2h: %s\n", device_c2h);

    if (verbose)
    {
        fprintf(stdout,
                "dev %s, addr 0x%lx, aperture 0x%lx, size 0x%lx, offset 0x%lx, "
                "count %lu\n",
                device_h2c, address, aperture, size, offset, count);
    }

    sprintf(filebuf, "%s_output_%ld.bin", in_file, current_timestamp());
    out_file = filebuf;

    ret = dma_to_device(device_h2c, address, aperture, size, 1, offset, count, in_file);
    if (ret < 0)
    {
        printf("dma_to_device ret < 0: %ld\n", ret);
    }
    size = ret;

    ret = dma_from_device(device_c2h, address, aperture, size, offset, count, out_file);
    if (ret < 0)
    {
        printf("dma_from_device ret < 0: %ld\n", ret);
    }
    printf("src: %s\n", in_file);
    printf("dest: %s\n", out_file);


    sprintf(buf, "diff %s %s -s -q", in_file, out_file);
    printf("%s\n", buf);
    ret = system(buf);
    printf("ret=%ld\n", ret);
}

/**
 * approach:
 * scan address space for magic value
 * read/write to page with process_vm_readv/write
 */

#define _GNU_SOURCE

#include <emmintrin.h>
#include "lib/scanmem/commands.h"

#include <scanmem/scanmem.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include "share.h"

#define LOG_VERBOSE 1
#include "log.h"

#define DATA_BUF_LEN 4096
volatile static struct data_t *data;
#define POINTER_FMT "%12lx"

typedef struct extract_region
{
    unsigned long num;
    unsigned long address;
    unsigned int region_id;
    unsigned long match_off;
    const char *region_type;
} extract_region_t;


inline void show_error(const char *fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    fprintf(stderr, "error: ");
    vfprintf(stderr, fmt, args);
    va_end (args);
}

inline static void process_vm_trace_error(size_t error)
{
    switch (error)
    {
        case EINVAL:
            printf("ERROR: INVALID ARGUMENTS.\n");
            break;
        case EFAULT:
            printf("ERROR: UNABLE TO ACCESS TARGET MEMORY ADDRESS.\n");
            break;
        case ENOMEM:
            printf("ERROR: UNABLE TO ALLOCATE MEMORY.\n");
            break;
        case EPERM:
            printf("ERROR: INSUFFICIENT PRIVILEGES TO TARGET PROCESS.\n");
            break;
        case ESRCH:
            printf("ERROR: PROCESS DOES NOT EXIST.\n");
            break;
        default:
            printf("ERROR: AN UNKNOWN ERROR HAS OCCURRED.\n");
    }
}

inline static void print_region(extract_region_t *r)
{
    fprintf(stdout, "[%2lu] "POINTER_FMT", %2u + "POINTER_FMT", %5s\n",
            r->num, r->address, r->region_id, r->match_off, r->region_type);
}

static extract_region_t *extract_regions(globals_t *vars)
{
    assert(vars->num_matches > 0);

    const char *region_names[] = REGION_TYPE_NAMES;
    unsigned long num = 0;
    element_t *np = NULL;

    extract_region_t *res = calloc(vars->num_matches,
                                   sizeof(extract_region_t));
    if (!res)
    {
        printf("malloc failed\n");
        exit(-1);
    }

    np = vars->regions->head;
    matches_and_old_values_swath *reading_swath_index = vars->matches->swaths;
    size_t reading_iterator = 0;

    while (reading_swath_index->first_byte_in_child)
    {
        match_flags flags = reading_swath_index->data[reading_iterator].match_info;
        if (flags != flags_empty)
        {
            void *address = remote_address_of_nth_element(reading_swath_index,
                                                          reading_iterator);
            unsigned long address_ul = (unsigned long) address;
            unsigned int region_id = 99;
            unsigned long match_off = 0;
            const char *region_type = "??";
            while (np)
            {
                region_t *region = np->data;
                unsigned long region_start = (unsigned long) region->start;
                if (address_ul < region_start + region->size &&
                    address_ul >= region_start)
                {
                    region_id = region->id;
                    match_off = address_ul - region->load_addr;
                    region_type = region_names[region->type];
                    break;
                }
                np = np->next;
            }

            extract_region_t *curr = &res[num];
            curr->num = num;
            curr->address = address_ul;
            curr->region_id = region_id;
            curr->match_off = match_off;
            curr->region_type = region_type;
            num++;
        }
        ++reading_iterator;
        if (reading_iterator >= reading_swath_index->number_of_bytes)
        {
            reading_swath_index = local_address_beyond_last_element(reading_swath_index);
            reading_iterator = 0;
        }
    }
    assert(vars->num_matches == num);
    return res;
}

static bool read_data(pid_t pid, uintptr_t remote_ptr, time_t timeout_s)
{
    struct iovec transfer_local[2];
    transfer_local[0].iov_base = data->data; // data
    transfer_local[0].iov_len = 4096 - 64;
    transfer_local[1].iov_base = (void *) &data->turn; // turn
    transfer_local[1].iov_len = 8;

    struct iovec transfer_remote[2];
    transfer_remote[0].iov_base = (void *) remote_ptr + 64; // data
    transfer_remote[0].iov_len = 4096 - 64;
    transfer_remote[1].iov_base = (void *) remote_ptr + 8; // data
    transfer_remote[1].iov_len = 8;

    ssize_t ret = 0;

    const bool no_timeout = timeout_s == 0;
    const time_t start_time = time(NULL);
    time_t now = time(NULL);

    bool read_success = false;
    data->turn = TURN_GUEST;

    while (no_timeout || now - start_time <= timeout_s)
    {
        now = time(NULL);
        ret = process_vm_readv(pid,
                               transfer_local,
                               2,
                               transfer_remote,
                               2,
                               0);
        if (ret < 0)
        {
            process_vm_trace_error(errno);
            continue;
        }
        _mm_mfence();
        _mm_lfence();
        _mm_sfence();

        if (data->turn == TURN_HOST)
        {
            read_success = true;
            break;
        }
    }
    return read_success;
}

static bool write_data(pid_t pid, uintptr_t remote_ptr)
{
    ssize_t ret = 0;

    struct iovec transfer_local[2];
    transfer_local[0].iov_base = data->data; // data
    transfer_local[0].iov_len = 4096 - 64;
    transfer_local[1].iov_base = (void *) &data->turn; // turn
    transfer_local[1].iov_len = 8;

    struct iovec transfer_remote[2];
    transfer_remote[0].iov_base = (void *) remote_ptr + 64; // data
    transfer_remote[0].iov_len = 4096 - 64;
    transfer_remote[1].iov_base = (void *) remote_ptr + 8; // data
    transfer_remote[1].iov_len = 8;

    int try = 0;
    bool write_success = false;
    while (!write_success && try < 10)
    {
#if 0
        printf("Sending message to client pid %d, addr "
               POINTER_FMT "\n",
               pid, remote_ptr);
#endif
        _mm_mfence();
        _mm_lfence();
        _mm_sfence();

        data->magic = MAGIC;
        data->turn = TURN_GUEST;


        _mm_mfence();
        _mm_lfence();
        _mm_sfence();

        ret = process_vm_writev(pid,
                                transfer_local,
                                2,
                                transfer_remote,
                                2,
                                0);
        if (ret >= 0)
        {
            write_success = true;
        } else
        {
            process_vm_trace_error(errno);
        }
        try++;
    }
    if (!write_success)
    {
        printf("process_vm_writev failed "
               POINTER_FMT "\n",
               remote_ptr);
    }
    return write_success;
}

static int handshake(pid_t pid, uintptr_t remote_ptr)
{
    assert(remote_ptr != 0);
    assert(pid != 0);
    const time_t timeout_s = 2;

    const int nonce = rand() % RAND_MAX - 2;

    printf("writing nonce: %d\n", nonce);
    *((int *) data->data) = nonce;
    _mm_mfence();

    bool write_success = write_data(pid, remote_ptr);
    if (!write_success)
    {
        printf("write failed\n");
        return false;
    }

    bool read_success = read_data(pid, remote_ptr, timeout_s);
    if (!read_success)
    {
        printf("timeout, read failed\n");
        return false;
    }
    int reply = *((int *) data->data);
    if (reply - 1 != nonce)
    {
        printf("Got wrong nonce:  %d\n", reply);
        return false;
    }

    return true;
}

static size_t scan_guest_memory(pid_t pid, const char *magic)
{
    globals_t *vars = &sm_globals;
    vars->target = pid;
    if (!sm_init())
    {
        printf("failed to init\n");
        return -1;
    }
    if (getuid() != 0)
    {
        printf("We need sudo rights to scan all regions\n");
    }
    if (sm_execcommand(vars, "reset") == false)
    {
        vars->target = 0;
    }
    if (sm_execcommand(vars, magic) == false)
    {
        printf("Magic not found in address space: " MAGIC_STR "\n");
        return -1;
    }
    size_t regions_size = vars->num_matches;
    extract_region_t *regions = extract_regions(vars);

    unsigned long target_address = 0;
    for (int i = 0; i < regions_size; i++)
    {
        extract_region_t *region = &regions[i];
        print_region(region);
        bool success = handshake(pid, region->address);
        if (success)
        {
            printf("Found region with running client:" POINTER_FMT "\n", region->address);
            target_address = region->address;
            break;
        }
    }

    if (target_address == 0)
    {
        printf("target not found\n");
    }

    free(regions);
    return target_address;
}


static void benchmark(pid_t pid, uintptr_t remote_ptr)
{
    time_t start = 0, end = 0;

    printf("Benchmark start\n");
    memset(data->data, 0, DATA_SIZE);
    size_t n = 0;
    size_t N = 4096;

    while (true)
    {
        read_data(pid, remote_ptr, 0);
        if (n >= N) {
            end = (double )clock()/CLOCKS_PER_SEC;
            break;
        }
        if (n == 0) {
            printf("start timer\n");
            start = (float)clock()/CLOCKS_PER_SEC;
        }
//
//        for (int i = 0; i < DATA_SIZE / sizeof (int); i++)
//        {
//            int *d = (int*)data->data;
//            *(d + i) += 10;
//        }
        if (write_data(pid, remote_ptr) == 0)
        {
            printf("failed\n");
            exit(-1);
        }

        if (n % 1000 == 0) {
            printf("%d\n", n);
        }
        n ++;

    }

    printf("iterations: %ld\n", N);
    printf("total: %ld\n", end - start);
    printf("mean: %lf\n", 1.0 * (end - start) / N);
}

int main(int argc, char **argv)
{
    time_t t;
    srand((unsigned) time(&t));

    if (argc < 2)
    {
        show_error("need pid\n");
        return -1;
    }
    pid_t pid = strtol(argv[1], NULL, 10);
    data = (struct data_t *) malloc(sizeof(struct data_t));
    if (!data)
    {
        printf("malloc failed\n");
        exit(-1);
    }
    assert(sizeof(struct data_t) == 4096L);
    memset(data, 0, 4096);
    data->magic = MAGIC;

    uintptr_t remote_ptr = scan_guest_memory(pid, MAGIC_STR);
    assert(remote_ptr != 0);

    printf("Identified remote buffer: 0x" POINTER_FMT "\n", remote_ptr);

    benchmark(pid, remote_ptr);

    free(data);
}
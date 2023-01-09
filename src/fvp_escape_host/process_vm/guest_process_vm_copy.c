#include <stdio.h>
#include <string.h>

#include <time.h>
#include <sys/mman.h>
#include<unistd.h>
#include "share.h"

volatile static struct data_t *data;

static void handshake()
{
    data->turn = TURN_HOST;
    printf("Waiting for message\n");
    while (data->turn != TURN_GUEST)
    {
    }

    int handshake = *((int *) data->data);
    printf("Got message from host: %d\n", handshake);
    handshake++;

    *((int *) data->data) = handshake;
    asm volatile("dmb sy");
    data->turn = TURN_HOST;
    asm volatile("dmb sy");
}

inline static void send_data()
{
    __builtin___clear_cache((char *) data, (char *) data + 4096);
    asm volatile("dmb sy");
    data->turn = TURN_HOST;
    asm volatile("dmb sy");
}

inline static void read_data()
{
     __builtin___clear_cache((char *) data, (char *) data + 4096);
    while (data->turn != TURN_GUEST)
    {
    }
     __builtin___clear_cache((char *) data, (char *) data + 4096);
}

static void benchmark()
{
    const int N = 4096 * 64;

    const int verify = 0;
    printf("start benchmark\n");
    struct data_t *cpy = malloc(4096);
    if (!cpy)
    {
        printf("error\n");
        return;
    }

    for (int a = 0; a < N; a++)
    {
        {
            volatile int *ptr = (volatile int *) data->data;
            volatile int *cpy_ptr = (volatile int *) cpy->data;

#if 0
            for (int i = 0; i < DATA_SIZE / sizeof(int); i += 1)
            {
                int d = rand() % RAND_MAX - 20;
                *(ptr + i) = d;
                *(cpy_ptr + i) = d;
            }
#endif
            if (verify)
            {
                for (int i = 0; i < DATA_SIZE / sizeof(int); i += 1)
                {
                    int p = *(ptr + i);
                    int c = *(cpy_ptr + i);
                    if (p != c)
                    {
                        printf("init data failed\n");
                        assert(0);
                    }
                }
                printf("sending\n");
            }
        }

        send_data();
        read_data();

        if (verify)
        {
            volatile int *ptr = (volatile int *) data->data;
            volatile int *cpy_ptr = (volatile int *) cpy->data;

            int success = 0;
            for (int i = 0; i < DATA_SIZE / sizeof(int); i += 1)
            {
                int p = *(ptr + i);
                int c = *(cpy_ptr + i) + 10;
                if (p != c)
                {
                    success++;
                    printf("error for int* data->data[%d], p!=c, p=%d, c=%d\n", i, p, c);
                }
            }
            printf("%d mistakes\n", success);
        }
    }

    volatile int *ptr = (volatile int *) data->data;
    volatile int *cpy_ptr = (volatile int *) cpy->data;

    int success = 0;
    for (int i = 0; i < DATA_SIZE / sizeof(int); i += 1)
    {
        int p = *(ptr + i);
        int c = *(cpy_ptr + i) + 10;
        if (p != c)
        {
            success++;
            printf("error for int* data->data[%d], p!=c, p=%d, c=%d\n", i, p, c);
        }
    }
    printf("%d mistakes\n", success);
}

int main(int argc, char *argv[])
{
    volatile size_t *_ptr = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE, -1, 0);
    if (_ptr == NULL)
    {
        printf("error\n");
        return -1;
    }
    memset((char *) _ptr, 0, 4096);
    data = (struct data_t *) _ptr;
    data->magic = MAGIC;

    handshake();

    printf("handshake done\n");
    sleep(2);
    benchmark();
}


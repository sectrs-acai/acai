//
// Created by b on 12/13/22.
//

#ifndef PROCESS_VM_ESCAPE_SHARE_H
#define PROCESS_VM_ESCAPE_SHARE_H

#include <sys/uio.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#define FILE_LINE __FILE__ ":" STR(__LINE__)
#define PTR_FMT "0x%llx"
#define HERE printf(FILE_LINE "\n")

#define DATA_SIZE 4032
struct __attribute__((__packed__))  data_t
{
    volatile unsigned long magic;
    volatile unsigned long turn;
    char padding[48];
    char data[DATA_SIZE];
};


static inline void hex_dump(const void *data, size_t size)
{
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i)
    {
        printf("%02X ", ((unsigned char *) data)[i]);
        if (((unsigned char *) data)[i] >= ' ' && ((unsigned char *) data)[i] <= '~')
        {
            ascii[i % 16] = ((unsigned char *) data)[i];
        } else
        {
            ascii[i % 16] = '.';
        }
        if ((i + 1) % 8 == 0 || i + 1 == size)
        {
            printf(" ");
            if ((i + 1) % 16 == 0)
            {
                printf("|  %s \n", ascii);
            } else if (i + 1 == size)
            {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8)
                {
                    printf(" ");
                }
                for (j = (i + 1) % 16; j < 16; ++j)
                {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}

#define MAGIC 0xABCDEFFECDABCDEF
#define MAGIC_STR "0xABCDEFFECDABCDEF"
#define TURN_GUEST 0x1
#define TURN_HOST 0x2
#define TURN_INIT 0x3


#endif //PROCESS_VM_ESCAPE_SHARE_H

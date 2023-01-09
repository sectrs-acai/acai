//
// Created by b on 1/1/23.
//

#ifndef KERNEL_MODULE_PAGESCAN_H
#define KERNEL_MODULE_PAGESCAN_H

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct extract_region
{
    unsigned long num;
    unsigned long address;
    unsigned int region_id;
    unsigned long match_off;
    const char *region_type_str;
    int region_type;
} extract_region_t;

void print_region(extract_region_t *r);
int scan_guest_memory(pid_t pid, const char *magic, extract_region_t **ret_regions,
                      int *ret_regions_len
);
#endif //KERNEL_MODULE_PAGESCAN_H

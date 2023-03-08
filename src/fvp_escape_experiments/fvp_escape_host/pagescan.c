
#include "pagescan.h"
#include <scanmem/commands.h>
#include <scanmem/scanmem.h>

#include <emmintrin.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define POINTER_FMT "%12lx"


void print_region(extract_region_t *r)
{
    fprintf(stdout, "[%2lu] "POINTER_FMT", %2u + "POINTER_FMT", %5s (%d)\n",
            r->num, r->address, r->region_id, r->match_off, r->region_type_str, r->region_type);
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
            const char *region_type_str = "??";
            int region_type = -1;
            while (np)
            {
                region_t *region = np->data;
                unsigned long region_start = (unsigned long) region->start;
                if (address_ul < region_start + region->size &&
                    address_ul >= region_start)
                {
                    region_id = region->id;
                    match_off = address_ul - region->load_addr;
                    region_type_str = region_names[region->type];
                    region_type = region->type;
                    break;
                }
                np = np->next;
            }

            extract_region_t *curr = &res[num];
            curr->num = num;
            curr->address = address_ul;
            curr->region_id = region_id;
            curr->match_off = match_off;
            curr->region_type_str = region_type_str;
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

int scan_guest_memory(pid_t pid, const char *magic, extract_region_t **ret_regions,
                      int *ret_regions_len
)
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
        printf("Magic not found in address space\n");
        return -1;
    }
    *ret_regions_len = vars->num_matches;
    *ret_regions = extract_regions(vars);
    sm_cleanup();

    return 0;
}


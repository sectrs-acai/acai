//
// Created by b on 1/12/23.
//

#ifndef TRUSTED_PERIPH_FAULTHOOKLIB_H
#define TRUSTED_PERIPH_FAULTHOOKLIB_H

#define FILE_LINE __FILE__ ":" STR(__LINE__)
#define PTR_FMT "0x%llx"

#define HERE printf("%s/%s: %d\n", __FILE__, __FUNCTION__, __LINE__)

#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_YELLOW "\x1b[33m"
#define COLOR_RESET "\x1b[0m"

#define TAG_OK COLOR_GREEN "[+]" COLOR_RESET " "
#define TAG_FAIL COLOR_RED "[-]" COLOR_RESET " "
#define TAG_PROGRESS COLOR_YELLOW "[~]" COLOR_RESET " "


#define POINTER_FMT "%12lx"
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

int scan_guest_memory(pid_t pid,
                      unsigned long magic,
                      extract_region_t **ret_regions,
                      int *ret_regions_len
);

#define FAULTHOOK_DIR "/sys/kernel/debug/faulthook/"


struct faulthook_struct
{
    bool is_init;
    pid_t target_pid;
    unsigned long magic;
    int fd_hook;
};
//
//extern struct faulthook_struct faulthook_ctx;
//
//void fh_init();
//void fh_cleanup();
//void fh_scan_memory()

// -------------------
// kernel api
// -------------------

#define FAULTHOOK_IOCTL_MAGIC_NUMBER (long)0x3d18

#define FAULTHOOK_IOCTL_CMD_STATUS \
  _IOR(FAULTHOOK_IOCTL_MAGIC_NUMBER, 1, size_t)

#define FAULTHOOK_IOCTL_CMD_HOOK_COMPLETED \
  _IOR(FAULTHOOK_IOCTL_MAGIC_NUMBER, 2, size_t)

struct faulthook_ctrl
{
    bool active;
    pid_t pid;
    unsigned long address;
    unsigned long len;
};

// -------------------

static inline void hex_dump(
    const void *data,
    size_t size
)
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

#endif //TRUSTED_PERIPH_FAULTHOOKLIB_H

#ifndef GPU_NOUVEAU_STUB_FH_DEF_H_
#define GPU_NOUVEAU_STUB_FH_DEF_H_

#include "gdev_ioctl_def.h"
#include "gdev_api.h"

struct faultdata_struct
{
    volatile unsigned long nonce; // fault
    unsigned long turn;
    unsigned long action;
    unsigned long data_size;
    char data[0];
};

struct faultdata_page_chunk
{
    unsigned long addr; // this is either pfn or addr
    unsigned long offset;
    unsigned long nbytes;
    int type;
};

enum fh_turn
{
    FH_TURN_HOST = 1,
    FH_TURN_GUEST = 2
};

enum fh_action
{
    FH_ACTION_NOACTION = 0,
    FH_ACTION_UNDEF = 1,
    FH_ACTION_GUEST_CONTINUE = 2,
    FH_ACTION_ALLOC_GUEST = 10,
    FH_ACTION_OPEN_DEVICE = 11,
    FH_ACTION_CLOSE_DEVICE = 12,
    FH_ACTION_MMAP = 13,
    FH_ACTION_READ = 14,
    FH_ACTION_WRITE = 15,
    FH_ACTION_UNMAP = 16,
    FH_ACTION_PING = 17,
    FH_ACTION_IOCTL_DMA = 18,
    FH_ACTION_VERIFY_MAPPING = 20,
    FH_ACTION_SETUP = 21,
    FH_ACTION_TEARDOWN = 22,
    FH_ACTION_DMA = 23,
    FH_ACTION_SEEK = 24,
    FH_ACTION_GET_EMPTY_MAPPINGS = 25,
    FH_ACTION_TRANSFER_ESCAPE_DATA = 30,
    FH_ACTION_IOCTL = 31,
};


#define ACTION_MODIFIER __attribute__((__packed__))


struct ACTION_MODIFIER fh_action_common
{
    unsigned long flags;
    int fd;
    int err_no;
    ssize_t ret;
};

struct ACTION_MODIFIER fh_action_open
{
    struct fh_action_common common;
    char device[128];
    unsigned int flags;
};

struct ACTION_MODIFIER fh_action_close
{
    struct fh_action_common common;
    char device[128];
};

struct fh_gdev_ioctl {
    struct fh_action_common common;
    unsigned long gdev_command;
    union {
        char payload[0];
        struct ACTION_MODIFIER ioctl_tune {
            struct gdev_ioctl_tune req;
        } gtune;
        struct ACTION_MODIFIER ioctl_query {
            struct gdev_ioctl_query req;
        } gquery;
        struct ACTION_MODIFIER ioctl_malloc {
            struct gdev_ioctl_mem req;
        } gmalloc;
        struct fh_ioctl_memcpy_to_device {
            struct gdev_ioctl_dma req;
            unsigned long src_buf_pfn_num;
            unsigned long *src_buf_pfn; // addr[0] memory following this struct is pages
        } gmemcpy_to_device;
        struct fh_ioctl_memcpy_from_device {
            struct gdev_ioctl_dma req;
            unsigned long dest_buf_pfn_num;
            unsigned long *dest_buf_pfn; // addr[0] memory following this struct is pages
        } gmemcpy_from_device;
        struct fh_ioctl_glaunch {
            struct gdev_ioctl_launch req;
            uint32_t id;
            struct gdev_kernel kernel;
            uint32_t kernel_param_size; /* kernel parameter size */
            uint32_t *kernel_param; // kernel_param[0]
        } glaunch;
        struct fh_ioctl_gsync {
            // struct gdev_ioctl_sync req;
            uint32_t id;
            struct gdev_time timeout;
            uint8_t has_timeout;
        } gsync;
        struct fh_ioctl_gfree {
            struct gdev_ioctl_mem req;
        } gfree;
    };
};


struct ACTION_MODIFIER fh_action_ping
{
    struct fh_action_common common;
    int ping;
};

enum fh_action_verify_mapping_status
{
    fh_action_verify_mapping_success = 1,
    fh_action_verify_mapping_fail = 2
};
struct ACTION_MODIFIER fh_action_verify_mapping
{
    struct fh_action_common common;
    size_t pfn;
};

struct ACTION_MODIFIER fh_action_read
{
    struct fh_action_common common;
    size_t count; /* how much to read */
    loff_t offset; /* offset */
    ssize_t buffer_size; /* how much actually read */
    char *buffer;
};

struct ACTION_MODIFIER fh_action_write
{
    struct fh_action_common common;
    size_t count; /* how much to write */
    loff_t offset; /* offset */
    ssize_t buffer_size; /* how much actually read */
    char *buffer;
};

struct ACTION_MODIFIER fh_action_mmap
{
    struct fh_action_common common;

    /* the offset starting from the base page in the fvp shared buffer */
    unsigned long vm_start;
    unsigned long vm_end;

    unsigned long vm_pgoff;
    unsigned long vm_flags;
    unsigned long vm_page_prot;
    unsigned long pfn_size;
    unsigned long pfn[0];
};

struct ACTION_MODIFIER fh_action_unmap
{
    struct fh_action_common common;
    unsigned long vm_start;
    unsigned long vm_end;
    unsigned long pfn_size;
    unsigned long pfn[0];
};

struct ACTION_MODIFIER fh_action_ioctl
{
    struct fh_action_common common;
    char name[64];
    unsigned long cmd;
    unsigned long arg_insize;
    unsigned long arg_outsize;
    char arg[0];

};

struct ACTION_MODIFIER action_seek
{
    struct fh_action_common common;
    loff_t off;
    int whence;
};

struct ACTION_MODIFIER page_chunk
{
    unsigned long addr; // this is either pfn or addr
    unsigned long offset;
    unsigned long nbytes;
};

#define action_dma_size(dma) sizeof(struct action_dma) + dma->pages_nr * sizeof(struct page_chunk)

struct ACTION_MODIFIER action_dma
{
    struct fh_action_common common;
    unsigned long phy_addr;
    int do_write; /* 1 is write, 0 is read */
    int do_aperture;
    char *user_buf;
    unsigned long len;
    unsigned long pages_nr;
    struct page_chunk page_chunks[0];
};

struct fh_host_ioctl_dma
{
    pid_t pid;
    unsigned long phy_addr;
    int do_write; /* 1 is write, 0 is read */
    int do_aperture;
    char *user_buf;
    unsigned long len;
    unsigned long chunks_nr;
    struct page_chunk * chunks;
};

struct ACTION_MODIFIER action_empty_mappings
{
    struct fh_action_common common;
    unsigned int last_pfn;
    unsigned long pfn_max_nr_guest;
    unsigned long pfn_nr;
    unsigned long pfn[0];
};


enum action_transfer_escape_data_status {
    action_transfer_escape_data_status_done = 0,
    action_transfer_escape_data_status_ok = 1,
    action_transfer_escape_data_status_transfer_size = 2,
    action_transfer_escape_data_status_transfer_data = 3,
    action_transfer_escape_data_status_error = 4,
};

struct ACTION_MODIFIER action_transfer_escape_data
{
    int status;
    union {
        struct ACTION_MODIFIER handshake {
            unsigned int total_size;
            unsigned int chunk_size;
            unsigned long info_ctx;
        } handshake;
        struct ACTION_MODIFIER chunk {
            unsigned int chunk_data_size;
            char chunk_data[0];
        } chunk;
    };
};

#endif //GPU_NOUVEAU_STUB_FH_DEF_H_

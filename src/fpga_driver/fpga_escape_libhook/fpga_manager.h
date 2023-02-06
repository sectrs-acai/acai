#ifndef TRUSTED_PERIPH_FPGA_MANAGER_H
#define TRUSTED_PERIPH_FPGA_MANAGER_H

#define FAULTDATA_MAGIC 0xAABBCCDDEEFF9984

struct __attribute__((__packed__))  faultdata_struct
{
    volatile unsigned long nonce; // fault
    volatile unsigned long turn;
    volatile unsigned long action;
    volatile unsigned long data_size;
    volatile char data[0];
};

typedef struct ctx_struct *ctx_struct;
unsigned long get_addr_map_vaddr(
        ctx_struct ctx, unsigned long pfn);

int on_fault(unsigned long addr,
             unsigned long len,
             pid_t target_pid,
             unsigned long target_addr,
             ctx_struct ctx);


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
};

#define PING_LEN 512

static inline const char *fh_action_to_str(int action)
{
    switch (action)
    {
        case FH_ACTION_UNDEF: return "undef";
        case FH_ACTION_GUEST_CONTINUE:return "FH_ACTION_GUEST_CONTINUE";
        case FH_ACTION_ALLOC_GUEST:return "FH_ACTION_ALLOC_GUEST";
        case FH_ACTION_OPEN_DEVICE:return "FH_ACTION_OPEN_DEVICE";
        case FH_ACTION_CLOSE_DEVICE:return "FH_ACTION_CLOSE_DEVICE";
        case FH_ACTION_MMAP:return "FH_ACTION_MMAP";
        case FH_ACTION_READ:return "FH_ACTION_READ";
        case FH_ACTION_WRITE:return "FH_ACTION_WRITE";
        case FH_ACTION_UNMAP:return "FH_ACTION_UNMAP";
        case FH_ACTION_PING: return "FH_ACTION_PING";
        case FH_ACTION_SETUP: return "FH_ACTION_SETUP";
        case FH_ACTION_TEARDOWN: return "FH_ACTION_TEARDOWN";
        case FH_ACTION_VERIFY_MAPPING: return "FH_ACTION_VERIFY_MAPPING";
        default:
        {
            return "unknown action";
        }
    }
}

#define ACTION_MODIFIER __attribute__((__packed__))

struct ACTION_MODIFIER action_common
{
    int fd;
    ssize_t ret;
    int err_no;
};

struct ACTION_MODIFIER action_openclose_device
{
    struct action_common common;
    char device[128];
    unsigned int flags;
};

struct ACTION_MODIFIER action_ping
{
    int ping;
};

enum action_verify_mappping_status {
    action_verify_mappping_success = 1,
    action_verify_mappping_fail = 2
};
struct ACTION_MODIFIER action_verify_mappping
{
    unsigned int status;
    size_t pfn;
};

struct ACTION_MODIFIER action_read
{
    struct action_common common;
    size_t count; /* how much to read */
    loff_t offset; /* offset */
    ssize_t buffer_size; /* how much actually read */
    char *buffer;
};

struct ACTION_MODIFIER action_write
{
    struct action_common common;
    size_t count; /* how much to write */
    loff_t offset; /* offset */
    ssize_t buffer_size; /* how much actually read */
    char *buffer;
};


struct ACTION_MODIFIER action_mmap_device
{
    struct action_common common;

    /* the offset starting from the base page in the fvp shared buffer */
    unsigned long mmap_guest_kernel_offset;
    unsigned long vm_start;
    unsigned long vm_end;

    unsigned long vm_pgoff;
    unsigned long vm_flags;
    unsigned long vm_page_prot;
    unsigned long pfn_size;
    unsigned long pfn[0];
};

struct ACTION_MODIFIER action_unmap
{
    struct action_common common;
    unsigned long vm_start;
    unsigned long vm_end;
    unsigned long pfn_size;
    unsigned long pfn[0];
};

struct ACTION_MODIFIER action_dma
{
    struct action_common common;
    // struct xdma_aperture_ioctl io;
    int write_read; // 1 is write
    char *buffer;
};


struct ACTION_MODIFIER action_init_guest
{
    struct action_common common;
    unsigned long host_offset;
};

struct ACTION_MODIFIER action_ioctl
{
    struct action_common common;
    unsigned int cmd;
    unsigned long arg;

};

#endif
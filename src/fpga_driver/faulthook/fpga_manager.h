#ifndef TRUSTED_PERIPH_FPGA_MANAGER_H
#define TRUSTED_PERIPH_FPGA_MANAGER_H

#define FAULTDATA_MAGIC 0xAABBCCDDEEFF9987

struct __attribute__((__packed__))  faultdata_struct {
    volatile unsigned long magic;
    volatile unsigned long nonce; // fault
    volatile unsigned long turn;
    volatile unsigned long length;
    volatile unsigned long action;
    char data[0];
};


enum fh_turn {
    FH_TURN_HOST = 1,
    FH_TURN_GUEST = 2
};

enum fh_action {
    FH_ACTION_UNDEF = 1,
    FH_ACTION_GUEST_CONTINUE = 2,
    FH_ACTION_ALLOC_GUEST = 10,
    FH_ACTION_OPEN_DEVICE = 11,
    FH_ACTION_CLOSE_DEVICE = 12,
    FH_ACTION_MMAP = 13
};


static inline const char *fh_action_to_str(int action)
{
    switch (action) {
        case FH_ACTION_UNDEF: return "undef";
        case FH_ACTION_GUEST_CONTINUE:return "FH_ACTION_GUEST_CONTINUE";
        case FH_ACTION_ALLOC_GUEST:return "FH_ACTION_ALLOC_GUEST";
        case FH_ACTION_OPEN_DEVICE:return "FH_ACTION_OPEN_DEVICE";
        case FH_ACTION_CLOSE_DEVICE:return "FH_ACTION_CLOSE_DEVICE";
        case FH_ACTION_MMAP:return "FH_ACTION_MMAP";
        default:return "unknown action";
    }
}

#define ACTION_MODIFIER __attribute__((__packed__))
struct ACTION_MODIFIER action_openclose_device {
    char device[128];
    unsigned int flags;
    int fd;
    int ret;
    int err_no;
};


struct ACTION_MODIFIER action_mmap_device {
    int ret;
    int err_no;
    int fd;
    unsigned long vm_start;
    unsigned long vm_end;
    unsigned long vm_pgoff;
    unsigned long vm_flags;
    unsigned long vm_page_prot;
    unsigned long mmap_guest_kernel_offset;
};


struct ACTION_MODIFIER action_init_guest {
    unsigned long host_offset;
};


#endif
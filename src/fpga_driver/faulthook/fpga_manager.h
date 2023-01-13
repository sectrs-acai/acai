#ifndef TRUSTED_PERIPH_FPGA_MANAGER_H
#define TRUSTED_PERIPH_FPGA_MANAGER_H

#define FAULTDATA_MAGIC 0xAABBCCDDEEFF9987

struct __attribute__((__packed__))  faultdata_struct {
    volatile unsigned long magic;
    volatile unsigned long nonce; // fault
    volatile unsigned long turn;
    volatile unsigned long length;
    volatile unsigned long data_host_page_offset;
    volatile unsigned long action;
    unsigned long padding[24];
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
};


static inline const char *fh_action_to_str(int action)
{
    switch (action) {
        case FH_ACTION_UNDEF: return "undef";
        case FH_ACTION_GUEST_CONTINUE:return "FH_ACTION_GUEST_CONTINUE";
        case FH_ACTION_ALLOC_GUEST:return "FH_ACTION_ALLOC_GUEST";
        case FH_ACTION_OPEN_DEVICE:return "FH_ACTION_OPEN_DEVICE";
        case FH_ACTION_CLOSE_DEVICE:return "FH_ACTION_CLOSE_DEVICE";
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


#endif
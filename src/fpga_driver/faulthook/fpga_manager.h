#ifndef TRUSTED_PERIPH_FPGA_MANAGER_H
#define TRUSTED_PERIPH_FPGA_MANAGER_H

#define FAULTDATA_MAGIC 0xAABBCCDDEEFF9987

struct __attribute__((__packed__))  faultdata_struct {
    volatile unsigned long magic;
    volatile unsigned long turn;
    volatile unsigned long length;
    volatile unsigned long data_host_page_offset;
    volatile unsigned long action;
    unsigned long padding[24];
    char data[0];
};

enum fh_action {
    FH_ACTION_UNDEF = -1,
    FH_ACTION_GUEST_CONTINUE = 1,
    FH_ACTION_ALLOC_GUEST = 10,
    FH_ACTION_OPEN_DEVICE = 11,
    FH_ACTION_CLOSE_DEVICE = 12,
};

#define ACTION_MODIFIER __attribute__((__packed__))
struct ACTION_MODIFIER action_openclose_device {
    char device[128];
    int fd;
    int ret;
};


#endif
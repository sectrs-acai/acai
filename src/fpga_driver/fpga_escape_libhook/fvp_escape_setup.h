#ifndef LINUX_FVP_ESCAPE_SETUP_H_
#define LINUX_FVP_ESCAPE_SETUP_H_

#define FVP_CONTROL_MAGIC 0xCAFECAFECAFECA00
#define FVP_ESCAPE_MAGIC 0xCAFECAFECAFECA01

enum fvp_escape_setup_turn {
    fvp_escape_setup_turn_guest = 1,
    fvp_escape_setup_turn_host = 2,
};

enum fvp_escape_setup_action {
    fvp_escape_setup_action_undef = 0,
    fvp_escape_setup_action_wait_guest = 1,
    fvp_escape_setup_action_continue_guest = 2,
    fvp_escape_setup_action_addr_mapping = 3,
    fvp_escape_setup_action_addr_mapping_success = 4,
};

struct __attribute__((__packed__)) fvp_escape_setup_struct {
    /* common for all tagged pages */
    volatile unsigned long ctrl_magic;
    volatile unsigned long addr_tag;

    /* escape */
    volatile unsigned long escape_magic;
    volatile unsigned long escape_turn;
    volatile unsigned long escape_hook;
    volatile unsigned long action;
    volatile unsigned long data_size;
    char data[0];
};

struct __attribute__((__packed__)) fvp_escape_scan_struct {
    volatile unsigned long ctrl_magic;
    volatile unsigned long addr_tag;
    volatile unsigned long escape_magic;
};

#endif

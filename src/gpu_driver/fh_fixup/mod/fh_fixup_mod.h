#ifndef GPU_FH_FIXUP_MOD_FH_FIXUPMOD_H_
#define GPU_FH_FIXUP_MOD_FH_FIXUPMOD_H_

#define FH_FIXUP_DEVICE_NAME "fh_fixup2"
#define FH_FIXUP_DEVICE_PATH "/dev/" FH_FIXUP_DEVICE_NAME

#define FH_FIXUP_IOCTL_MAGIC_NUMBER (long)0x7d19

#define FH_FIXUP_IOCTL_MAP \
  _IOR(FH_FIXUP_IOCTL_MAGIC_NUMBER, 1, size_t)

#define FH_FIXUP_IOCTL_UNMAP \
  _IOR(FH_FIXUP_IOCTL_MAGIC_NUMBER, 2, size_t)

typedef struct {
    void *src_base_addr;
    size_t pages_num;
    pid_t target_pid;
    void** target_pages;
    unsigned long handle;
} fh_fixup_map_t;

#endif //GPU_FH_FIXUP_MOD_FH_FIXUPMOD_H_


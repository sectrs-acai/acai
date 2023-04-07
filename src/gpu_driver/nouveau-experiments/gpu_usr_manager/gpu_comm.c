#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <errno.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <drm/nouveau_drm.h>
#include "fh_def.h"
#include "usr_manager.h"
#include "drm.h"
#include <stdint.h>
#include "nouveau_abi16.h"


#define IOCTL_DEBUG 1


// overwrite request to this device
#define GPU_DEVICE_OVERRIDE 1
#define GPU_DEVICE "/dev/dri/renderD128"

#define set_ret_and_err_no(a, _r) \
    print_progress("ret: %d, errno: %d\n", _r, _r< 0 ? errno: 0) ; \
    if (_r < 0)perror("");                              \
    a->common.flags = 0;                              \
    a->common.fd = _r; \
    a->common.ret = _r; \
    a->common.err_no = _r < 0 ? errno:0

#define set_ret_and_err_no_direct(a, _r) \
    a->common.fd = _r; \
    a->common.ret = _r; \
    a->common.err_no = _r

static inline void hex_dump(
        const void *data,
        size_t size
)
{
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++ i)
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
                for (j = (i + 1) % 16; j < 16; ++ j)
                {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}


static inline const char *fh_action_to_str(int action)
{
    switch (action)
    {
        default:
        {
        }
    }
}


int generic_ioctl(struct faultdata_struct *fault)
{
    struct fh_action_ioctl *a = (struct fh_action_ioctl *) fault->data;
    int ret = 0;
    void *ioctl_data = malloc(MAX(a->arg_insize, a->arg_outsize));
    if (ioctl_data == NULL)
    {
        print_err("malloc OOM\n");
        return - ENOMEM;
    }
    memcpy(ioctl_data, a->arg, a->arg_insize);
    ret = ioctl(a->common.fd, a->cmd, ioctl_data);
    if (ret < 0 || IOCTL_DEBUG)
    {
        print_progress("in: \n");
        hex_dump(a->arg, a->arg_insize);
        print_progress("out: \n");
        hex_dump(ioctl_data, a->arg_outsize);
    }
    memcpy(a->arg, ioctl_data, a->arg_outsize);
    free(ioctl_data);
    set_ret_and_err_no(a, ret);
    return 0;
}

struct ioctl_gem_pushbuf {
    struct drm_nouveau_gem_pushbuf data;
};

int on_fault(unsigned long addr,
             unsigned long len,
             pid_t pid,
             unsigned long target_addr,
             ctx_struct ctx)
{
    struct faultdata_struct *fault = (struct faultdata_struct *) addr;
    if (fault->turn != FH_TURN_HOST)
    {
        /*
         * fault was caused unintentionally
         */
        #if 0
        print_err("turn is not FH_TURN_HOST\n");
        #endif
        return 0;
    }
    switch (fault->action)
    {
        case FH_ACTION_SETUP:
        {
            break;
        }
        case FH_ACTION_TEARDOWN:
        {
            break;
        }
        case FH_ACTION_PING:
        {
            break;
        }
        case FH_ACTION_OPEN_DEVICE:
        {
            print_ok("FH_ACTION_OPEN_DEVICE\n");
            struct fh_action_open *a = (struct fh_action_open *) fault->data;
            const char *dev = (const char *) GPU_DEVICE_OVERRIDE ? GPU_DEVICE:a->device;
            print_ok("open %s %d\n", dev, a->flags);
            int fd = open(dev, O_RDWR);
            HERE;
            a->common.fd = fd;
            set_ret_and_err_no(a, fd);
            break;
        }
        case FH_ACTION_CLOSE_DEVICE:
        {
            print_ok("FH_ACTION_CLOSE_DEVICE\n");
            struct fh_action_close *a = (struct fh_action_close *) fault->data;
            int ret = close(a->common.fd);
            set_ret_and_err_no(a, ret);
            break;
        }
        case FH_ACTION_IOCTL:
        {
            int ret;
            struct fh_action_ioctl *a = (struct fh_action_ioctl *) fault->data;
            print_ok(COLOR_GREEN "FH_ACTION_IOCTL %s %lx\n" COLOR_RESET, a->name, a->cmd);
            switch (a->cmd)
            {
                case DRM_IOCTL_NOUVEAU_GETPARAM:
                case DRM_IOCTL_NOUVEAU_CHANNEL_ALLOC:
                    generic_ioctl(fault);
                    break;
                case DRM_IOCTL_NOUVEAU_GEM_PUSHBUF: {
                    void *ioctl_data = malloc(MAX(a->arg_insize, a->arg_outsize));
                    if (ioctl_data == NULL)
                    {
                        print_err("malloc OOM\n");
                        return - ENOMEM;
                    }
                    memcpy(ioctl_data, a->arg, a->arg_insize);
                    struct drm_nouveau_gem_pushbuf *data = (ioctl_data);

                    free(ioctl_data);
                }
                default: {
                    print_err("unsupprted ioctl\n");
                    set_ret_and_err_no_direct(a, -EINVAL);
                }
            }
            break;
        }
        default:
        {
            print_err("unknown action code: %ld\n", fault->action);
        }
    }
    fault->turn = FH_TURN_GUEST;
    return 0;
}


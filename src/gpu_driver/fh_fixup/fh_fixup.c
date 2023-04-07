#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "mod/fh_fixup_mod.h"
#include "fh_fixup.h"
#include <sys/ioctl.h>
#include <unistd.h>
#include <sys/mman.h>

static int fh_fixup_fd;

fh_fixup_fn int fh_fixup_init()
{
    fh_fixup_fd = open(FH_FIXUP_DEVICE_PATH, O_RDONLY);
    if (fh_fixup_fd < 0) {
        printf("error: cannot open: %s\n", FH_FIXUP_DEVICE_PATH);
        return - 1;
    }
    return 0;
}

fh_fixup_fn void fh_fixup_cleanup()
{
    if (fh_fixup_fd >= 0) {
        close(fh_fixup_fd);
    }
}


/* target pages cant be copy on write */
fh_fixup_fn int fh_fixup_map(void *src_base,
                             unsigned long num_pages,
                             pid_t target_pid,
                             void **target_pages,
                             fh_fixup_map_ctx_t **ret_ctx)
{
    fh_fixup_map_t *req = malloc(sizeof (fh_fixup_map_t));
    if (req == NULL) {
        return -1;
    }
    int ret;

    for(int i = 0; i < num_pages; i ++) {
        volatile char *c =  (volatile char*) src_base + i * 4096;
        *c;
    }

    req->src_base_addr = src_base;
    req->pages_num = num_pages;
    req->target_pages = target_pages;
    req->target_pid = target_pid;
    ret = ioctl(fh_fixup_fd, FH_FIXUP_IOCTL_MAP, (size_t) req);
    if (ret < 0) {
        return ret;
    }
    *ret_ctx = malloc(sizeof(struct fh_fixup_map_ctx));
    (*ret_ctx)->req = req;

    return 0;
}

fh_fixup_fn int fh_fixup_unmap(fh_fixup_map_ctx_t *ctx)
{
    int ret;
    ret = ioctl(fh_fixup_fd, FH_FIXUP_IOCTL_UNMAP, (size_t) ctx->req);
    free(ctx->req);
    free(ctx);
    return ret;
}
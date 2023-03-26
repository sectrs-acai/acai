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
#include <stdint.h>
#include <assert.h>
#include "gdev_api.h"
#include "fh_def.h"
#include "usr_manager.h"
#include "gdev_ioctl_debug.h"

#define IOCTL_DEBUG 1

#define set_ret_and_err_no(a, _r) \
    print_progress("ret: %d, errno: %d\n", _r, _r< 0 ? errno: 0) ; \
    if (_r < 0)perror("");                              \
    a->common.flags = 0;                              \
    a->common.fd = _r; \
    a->common.ret = _r; \
    a->common.err_no = _r < 0 ? errno:0

#define set_ret_and_err_no_direct(a, _r) \
    if (_r < 0) {print_progress("ret: %d\n", _r );}; \
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
    for (i = 0; i < size; ++ i) {
        printf("%02X ", ((unsigned char *) data)[i]);
        if (((unsigned char *) data)[i] >= ' ' && ((unsigned char *) data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char *) data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i + 1) % 8 == 0 || i + 1 == size) {
            printf(" ");
            if ((i + 1) % 16 == 0) {
                printf("|  %s \n", ascii);
            } else if (i + 1 == size) {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8) {
                    printf(" ");
                }
                for (j = (i + 1) % 16; j < 16; ++ j) {
                    printf("   ");
                }
                printf("|  %s \n", ascii);
            }
        }
    }
}


static inline const char *fh_action_to_str(int action)
{
    switch (action) {
        default: {
        }
    }
}


int generic_ioctl(struct faultdata_struct *fault)
{
    struct fh_action_ioctl *a = (struct fh_action_ioctl *) fault->data;
    int ret = 0;
    void *ioctl_data = malloc(MAX(a->arg_insize, a->arg_outsize));
    if (ioctl_data == NULL) {
        print_err("malloc OOM\n");
        return - ENOMEM;
    }
    memcpy(ioctl_data, a->arg, a->arg_insize);
    ret = ioctl(a->common.fd, a->cmd, ioctl_data);
    if (ret < 0 || IOCTL_DEBUG) {
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

#define gdev_handles_max 512
Ghandle gdev_handles[gdev_handles_max];
int gdev_handle_i = 0;

static Ghandle get_gdev_handle(int index)
{
    if (index < 0 || index >= gdev_handle_i) {
        print_err("Invalid handle: no handle for index: %d\n");
        return NULL;
    }
    return gdev_handles[index];
}

int on_fault(unsigned long addr,
             unsigned long len,
             pid_t pid,
             unsigned long target_addr,
             ctx_struct ctx)
{
    struct faultdata_struct *fault = (struct faultdata_struct *) addr;
    if (fault->turn != FH_TURN_HOST) {
        /*
         * fault was caused unintentionally
         */
        #if 0
        print_err("turn is not FH_TURN_HOST\n");
        #endif
        return 0;
    }
    switch (fault->action) {
        case FH_ACTION_SETUP: {
            print_ok("FH_ACTION_SETUP\n");
            for (int i = 0; i < gdev_handles_max; i ++) {
                gdev_handles[i] = NULL;
            }
            gdev_handle_i = 0;
            break;
        }
        case FH_ACTION_TEARDOWN: {
            print_ok("FH_ACTION_TEARDOWN\n");
            for (int i = 0; i < gdev_handles_max; i ++) {
                if (gdev_handles[i] != NULL) {
                    gclose(gdev_handles[i]);
                    gdev_handles[i] = NULL;
                }
                gdev_handle_i = 0;
            }
        }
        case FH_ACTION_PING: {
            break;
        }
        case FH_ACTION_OPEN_DEVICE: {
            int ret = 0;
            print_ok("FH_ACTION_OPEN_DEVICE\n");
            struct fh_action_open *a = (struct fh_action_open *) fault->data;
            print_ok("open %s %d\n", a->device, a->flags);
            Ghandle h = gopen(0);
            if (h == NULL) {
                ret = - EINVAL;
            } else {
                a->common.fd = gdev_handle_i;
                gdev_handles[gdev_handle_i] = h;
                gdev_handle_i ++;
                ret = 0;
            }
            set_ret_and_err_no_direct(a, ret);
            break;
        }
        case FH_ACTION_CLOSE_DEVICE: {
            int ret = 0;
            print_ok("FH_ACTION_CLOSE_DEVICE\n");
            struct fh_action_close *a = (struct fh_action_close *) fault->data;
            Ghandle handle = get_gdev_handle(a->common.fd);
            if (handle == NULL) {
                set_ret_and_err_no_direct(a, - EINVAL);
                break;
            }
            ret = gclose(handle);
            gdev_handles[a->common.fd] = NULL;
            set_ret_and_err_no_direct(a, ret);
            break;
        }
        case FH_ACTION_IOCTL: {
            ssize_t ret = 0;
            struct fh_gdev_ioctl *a = (struct fh_gdev_ioctl *) fault->data;
            print_ok("FH_ACTION_IOCTL: %s\n", debug_ioctl_cmd_name(a->gdev_command));
            Ghandle handle = get_gdev_handle(a->common.fd);
            if (handle == NULL) {
                set_ret_and_err_no_direct(a, - EINVAL);
                break;
            }

            switch (a->gdev_command) {
                case GDEV_IOCTL_GTUNE: {
                    ret = gtune(handle, a->gtune.req.type, a->gtune.req.value);
                    debug_gdev_ioctl_tune(a->gtune.req);
                    break;
                }
                case GDEV_IOCTL_GQUERY: {
                    ret = gquery(handle, a->gquery.req.type, &a->gquery.req.result);
                    debug_gdev_ioctl_query(a->gquery.req);
                    break;
                }
                case GDEV_IOCTL_GMALLOC: {
                    a->gmalloc.req.addr = gmalloc(handle, a->gmalloc.req.size);
                    debug_gdev_ioctl_mem(a->gmalloc.req);
                    ret = 0;
                    break;
                }
                case GDEV_IOCTL_GFREE: {
                    a->gmalloc.req.size = gfree(handle, a->gfree.req.addr);
                    debug_gdev_ioctl_mem(a->gfree.req);
                    ret = 0;
                    break;
                }
                case GDEV_IOCTL_GMEMCPY_TO_DEVICE: {
                    unsigned long i = 0;
                    unsigned long nbytes_copied = 0;
                    unsigned long nbytes_per_page = 0;
                    unsigned long vaddr_fvp;
                    unsigned long *src_buf_pfn;
                    const unsigned long nbytes_tot = a->gmemcpy_to_device.req.size;
                    const unsigned long dma_dst_addr = a->gmemcpy_to_device.req.dst_addr;

                    char *target_buf = malloc(a->gmemcpy_to_device.req.size);
                    if (target_buf == NULL) {
                        ret = - ENOMEM;
                        break;
                    }
                    src_buf_pfn = (unsigned long *) &a->gmemcpy_to_device.src_buf_pfn;

                    printf("size: %ld, dst_addr: %lx, src_buf_pfn_size: %d, src_buf_pfn: %lx\n",
                           nbytes_tot,
                           dma_dst_addr,
                           a->gmemcpy_to_device.src_buf_pfn_num,
                           src_buf_pfn);


                    for (i = 0; i < a->gmemcpy_to_device.src_buf_pfn_num; i ++) {
                        print_progress("i: %d\n", i);
                        nbytes_per_page = MIN(nbytes_tot - nbytes_copied, 4096);
                        vaddr_fvp = get_addr_map_vaddr(ctx, src_buf_pfn[i]);

                        print_progress("i: %d, nbytes_pp %d, vaddr_fvp: %lx\n",
                                       i,
                                       nbytes_per_page,
                                       vaddr_fvp);
                        ret = copy_from_target(ctx,
                                               vaddr_fvp,
                                               nbytes_per_page,
                                               target_buf + nbytes_copied);


                        if (ret < nbytes_per_page) {
                            print_err("only copied: %ld bytes instead of %ld\n", ret,
                                      nbytes_per_page);
                            ret = - ENOMEM;
                            goto memcpy_to_dev_cleanup;
                        }
                        if (ret < 0) {
                            print_err("copy failed at vaddr_fvp: %lx\n", vaddr_fvp);
                            goto memcpy_to_dev_cleanup;
                        }

                        nbytes_copied += nbytes_per_page;
                    }
                    ret = gmemcpy_to_device(handle, dma_dst_addr, target_buf, nbytes_tot);
                    if (ret < 0) {
                        print_err("gmemcpy_to_device failed with %d\n", ret);
                    }
                    memcpy_to_dev_cleanup:
                    free(target_buf);
                    break;
                }
                case GDEV_IOCTL_GMEMCPY_FROM_DEVICE: {
                    unsigned long i = 0;
                    unsigned long nbytes_copied = 0;
                    unsigned long nbytes_per_page = 0;
                    struct fh_ioctl_memcpy_from_device *memcpy_from_device = &a->gmemcpy_from_device;
                    const unsigned long nbytes_tot = memcpy_from_device->req.size;
                    unsigned long vaddr_fvp;
                    const unsigned long dma_src_addr = memcpy_from_device->req.src_addr;
                    unsigned long *buf_pfn;

                    char *dma_buf = malloc(nbytes_tot);
                    if (dma_buf == NULL) {
                        ret = - ENOMEM;
                        break;
                    }

                    ret = gmemcpy_from_device(handle, dma_buf, dma_src_addr, nbytes_tot);
                    if (ret < 0) {
                        print_err("gmemcpy_from_device failed with %d\n", ret);
                        goto memcpy_from_dev_cleanup;
                    }

                    printf("size: %ld, dst_addr: %lx, src_buf_pfn_size: %d, src_buf_pfn: %lx\n",
                           nbytes_tot,
                           dma_src_addr,
                           memcpy_from_device->dest_buf_pfn_num,
                           buf_pfn);

                    buf_pfn = (unsigned long *) &memcpy_from_device->dest_buf_pfn;

                    for (i = 0; i < memcpy_from_device->dest_buf_pfn_num; i ++) {
                        print_progress("i: %d\n", i);
                        nbytes_per_page = MIN(nbytes_tot - nbytes_copied, 4096);
                        vaddr_fvp = get_addr_map_vaddr(ctx, buf_pfn[i]);

                        print_progress("i: %d, nbytes_pp %d, vaddr_fvp: %lx\n",
                                       i,
                                       nbytes_per_page,
                                       vaddr_fvp);
                        ret = copy_to_target(ctx,
                                             dma_buf + nbytes_copied,
                                             nbytes_per_page,
                                             vaddr_fvp);
                        if (ret < nbytes_per_page) {
                            print_err("only wrote: %ld bytes instead of %ld\n", ret,
                                      nbytes_per_page);
                            ret = - ENOMEM;
                            goto memcpy_from_dev_cleanup;
                        }
                        if (ret < 0) {
                            print_err("copy failed at vaddr_fvp: %lx\n", vaddr_fvp);
                            goto memcpy_from_dev_cleanup;
                        }

                        nbytes_copied += nbytes_per_page;
                    }
                    memcpy_from_dev_cleanup:
                    free(dma_buf);
                    break;
                }
                case GDEV_IOCTL_GLAUNCH: {
                    const char name[16] = "dummy";
                    struct gdev_kernel *kernel = malloc(sizeof(struct gdev_kernel));
                    const unsigned long param_buf_size = a->glaunch.kernel_param_size;
                    uint32_t *param_buf = malloc(param_buf_size);
                    if (! kernel || ! param_buf) {
                        ret = - ENOMEM;
                        break;
                    }
                    memcpy(kernel, &a->glaunch.kernel, sizeof(struct gdev_kernel));
                    memcpy(param_buf, (uint32_t *) &a->glaunch.kernel_param, param_buf_size);
                    kernel->param_buf = param_buf;
                    kernel->name = (char *) name;

                    ret = glaunch(handle, kernel, &a->glaunch.id);
                    print_progress("glaunch id: %d\n", a->glaunch.id);
                    free(kernel);
                    free(param_buf);
                    break;
                }
                case GDEV_IOCTL_GSYNC: {
                    struct gdev_time *timeout = a->gsync.has_timeout ? &a->gsync.timeout:NULL;
                    const uint32_t id = a->gsync.id;
                    ret = gsync(handle, id, timeout);
                    break;
                }
                case GDEV_IOCTL_GBARRIER: {
                    // HERE;
                    // print_err("GDEV_IOCTL_GBARRIER not yet impl\n");
                    //  TODO this makes everything hang
//                    ret = 0;
                    #if 1
                    ret = gbarrier(handle);
                    #endif
                    break;
                }
                default: {
                    print_err("not supported gdev_ioctl: %ld\n", a->gdev_command);
                    ret = - EINVAL;
                }
            }
            set_ret_and_err_no_direct(a, ret);
            break;
        }
        default: {
            print_err("unknown action code: %ld\n", fault->action);
        }
    }
    fault->turn = FH_TURN_GUEST;
    return 0;
}


#ifndef GPU_FH_FIXUP_FH_FIXUPLIB_H_
#define GPU_FH_FIXUP_FH_FIXUPLIB_H_

#define fh_fixup_fn

#include "mod/fh_fixup_mod.h"
#include <sys/types.h>

fh_fixup_fn int fh_fixup_init();
fh_fixup_fn void fh_fixup_cleanup();


struct fh_fixup_map_ctx {
    void **src_pages;
    void **target_pages;
    unsigned long num_pages;
    pid_t target_pid;
    unsigned long handle;
};

typedef struct fh_fixup_map_ctx fh_fixup_map_ctx_t;
/*
 * map a list of src_pages of size num_pages in current address space
 * into address space of target_pid at locations target_pages
 */
fh_fixup_fn int fh_fixup_map(void **src_pages,
                            unsigned long num_pages,
                            pid_t target_pid,
                            void** target_pages,
                            fh_fixup_map_ctx_t **ret_ctx);

fh_fixup_fn int fh_fixup_unmap(fh_fixup_map_ctx_t *ctx);

#endif //GPU_FH_FIXUP_FH_FIXUPLIB_H_

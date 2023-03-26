#ifndef XDMA__FVP_ESCAPE_H_
#define XDMA__FVP_ESCAPE_H_

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include "fh_def.h"

struct fh_ctx
{
    struct faultdata_struct *fh_escape_data;
    spinlock_t fh_lock;
    unsigned long fh_nonce;
    void *escape_ptr;
    unsigned long escape_size;
    unsigned long priv;
};


#define fh_ctx_t struct fh_ctx

struct __attribute__((__packed__)) pin_pages_struct
{
    char *user_buf;
    unsigned long len;
    struct page **pages;
    unsigned long pages_nr;
    unsigned long priv_data; /* for dma simulation, pointer to data */
    struct faultdata_page_chunk page_chunks[0];
};

static inline void fh_memcpy_escape_buf(
        fh_ctx_t *fh_ctx,
        void *target,
        void *src,
        unsigned int size,
        unsigned int header_size)
{
    unsigned long tot_copy = header_size + size + sizeof(struct faultdata_struct);
    memcpy(target, src, size);

    if (tot_copy > fh_ctx->escape_size)
    {
        pr_info("memcpy escape buf overflow\n");
        pr_info("escape buf: %ld, copy: %ld\n", fh_ctx->escape_size, tot_copy);
        BUG();
    }
}


#define fd_data (fh_ctx->fh_escape_data)

static inline void fd_data_lock(fh_ctx_t *fh_ctx)
{
    spin_lock(&fh_ctx->fh_lock);
}

static inline void fd_data_unlock(fh_ctx_t *fh_ctx)
{
    spin_unlock(&fh_ctx->fh_lock);
}

struct mmap_info
{
    void *data;
    unsigned long data_size;
    unsigned long order;
};

struct fh_fop_data
{
    int fd;
    void *private_data;
};

static inline void *fh_fop_get_private_data(struct file *file)
{
    struct fh_fop_data *p = (struct fh_fop_data *) file->private_data;
    if (p != NULL)
    {
        return p->private_data;
    }
    return NULL;
}

static inline struct fh_fop_data * fh_fop_get_data(struct file *file)
{
    return  (struct fh_fop_data *) file->private_data;
}

int fh_init(fh_ctx_t **ret_fh_ctx,
            void *escape_ptr,
            unsigned long escape_size);

int fh_cleanup(fh_ctx_t *fh_ctx);

int fh_do_escape(fh_ctx_t *fh_ctx, int action);

int fh_fop_open(fh_ctx_t *fh_ctx, struct inode *inode, struct file *file,
                void *private_data);

ssize_t fh_fop_read(fh_ctx_t *fh_ctx,
                    struct file *fp,
                    char __user *buf,
                    size_t count,
                    loff_t *pos);

ssize_t fh_fop_write(fh_ctx_t *fh_ctx,
                     struct file *file,
                     const char __user *buf,
                     size_t count,
                     loff_t *pos);

int fh_fop_close(fh_ctx_t *fh_ctx,
                 struct inode *dev_node,
                 struct file *filep);

int fh_fop_mmap(fh_ctx_t *fh_ctx,
                struct file *file,
                struct vm_area_struct *vma);


int fh_pin_pages(fh_ctx_t *fh_ctx,
                 const char __user *buf,
                 size_t count,
                 struct pin_pages_struct **ret_pages);

int fh_unpin_pages(fh_ctx_t *fh_ctx,
                   struct pin_pages_struct *pinned,
                   int do_free,
                   bool do_write);

inline unsigned long fh_get_page_count(fh_ctx_t *fh_ctx,
                                       const char __user *buf,
                                       size_t len);

#endif //XDMA__FVP_ESCAPE_H_

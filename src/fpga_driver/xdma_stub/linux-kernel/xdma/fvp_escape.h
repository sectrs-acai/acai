#ifndef XDMA__FVP_ESCAPE_H_
#define XDMA__FVP_ESCAPE_H_

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <fpga_escape_libhook/fpga_manager.h>

#include <asm/cacheflush.h>

#define HERE pr_info("%s/%s: %d\n", __FILE__, __FUNCTION__, __LINE__)
#define NOT_SUPPORTED pr_alert("Operation not supported: %s/%s: %d\n", __FILE__, __FUNCTION__, __LINE__)
#define PTR_FMT "0x%llx"

#if defined(__x86_64__) || defined(_M_X64)

#define faultdata_flush(faultdata) \
flush_cache_all()

#else
#define faultdata_flush(faultdata) \
asm volatile("dmb sy"); flush_cache_all()
#endif

struct __attribute__((__packed__)) pin_pages_struct
{
    char *user_buf;
    unsigned long len;
    struct page **pages;
    unsigned long pages_nr;
    struct page_chunk page_chunks[0];
};

extern spinlock_t faultdata_lock;

struct faultdata_driver_struct
{
    struct faultdata_struct *fd_data;
    struct page *page;

    unsigned long page_order;
    unsigned long fh_nonce;
    unsigned long host_pg_offset;
    unsigned long mmap_page; /* address to mapped page of mmap operation  (this->page with some offset) */
    unsigned long mmap_page_len;
    bool mmap_busy;
};
#define fd_data (fd_ctx.fd_data)

static inline void fd_data_lock(void) {
    spin_lock(&faultdata_lock);
}
static inline void fd_data_unlock(void) {
    spin_unlock(&faultdata_lock);
}

struct mmap_info
{
    void *data;
    unsigned long data_size;
    unsigned long order;
};

extern struct faultdata_driver_struct fd_ctx;

struct faulthook_priv_data
{
    int fd;
};

extern unsigned long *fvp_escape_page;
extern unsigned long fvp_escape_size;

int faulthook_cleanup(void);

int fh_do_faulthook(int action);

int faulthook_init(void);

int fh_char_close(struct inode *inode, struct file *file);

int fh_char_open(struct inode *inode, struct file *file);

ssize_t fh_char_ctrl_read(struct file *fp,
                          char __user *buf,
                          size_t count,
                          loff_t *pos);

ssize_t fh_char_ctrl_write(struct file *file, const char __user *buf,
                           size_t count, loff_t *pos);

long fh_char_ctrl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

int fh_bridge_mmap(struct file *file, struct vm_area_struct *vma);

int fh_unpin_pages(struct pin_pages_struct *pinned, int do_free, bool do_write);

inline unsigned long fh_get_page_count(const char __user *buf, size_t len);
int fh_pin_pages(const char __user *buf, size_t count,
                     struct pin_pages_struct **ret_pages);

extern ulong param_escape_page;
extern ulong param_escape_size;
extern int param_do_verify;

#endif //XDMA__FVP_ESCAPE_H_

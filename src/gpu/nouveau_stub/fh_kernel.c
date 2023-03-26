#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/ioctl.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/kthread.h>
#include <linux/kallsyms.h>
#include <linux/vmalloc.h>
#include <linux/types.h>
#include <linux/highmem.h>
#include "fh_kernel.h"

#define HERE pr_info("%s/%s: %d\n", __FILE__, __FUNCTION__, __LINE__)

#if defined(__x86_64__) || defined(_M_X64)
#define fh_flush \
flush_cache_all()

#else
#define fh_flush \
asm volatile("dmb sy"); flush_cache_all()
#endif


struct fh_ctx fd_ctx;
DEFINE_SPINLOCK(faultdata_lock);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
#define KPROBE_KALLSYMS_LOOKUP 1

typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);

kallsyms_lookup_name_t kallsyms_lookup_name_func;
#define kallsyms_lookup_name kallsyms_lookup_name_func

static struct kprobe kp = {
        .symbol_name = "kallsyms_lookup_name"
};
#endif

static int (*_soft_offline_page)(unsigned long pfn, int flags);

static bool (*_take_page_off_buddy)(struct page *page) = NULL;

static bool (*_is_free_buddy_page)(struct page *page) = NULL;

static int setup_lookup(void)
{
    #ifdef KPROBE_KALLSYMS_LOOKUP
    register_kprobe(&kp);
    kallsyms_lookup_name = (kallsyms_lookup_name_t) kp.addr;
    unregister_kprobe(&kp);

    if (! unlikely(kallsyms_lookup_name))
    {
        pr_alert("Could not retrieve kallsyms_lookup_name address\n");
        return - ENXIO;
    }
    #endif
    _soft_offline_page = (void *) kallsyms_lookup_name("soft_offline_page");
    if (_soft_offline_page == NULL)
    {
        pr_info("lookup failed soft_offline_page\n");
        return - ENXIO;
    }
    _take_page_off_buddy = (void *) kallsyms_lookup_name("take_page_off_buddy");
    if (_take_page_off_buddy == NULL)
    {
        pr_info("lookup failed _take_page_off_buddy\n");
        return - ENXIO;
    }
    _is_free_buddy_page = (void *) kallsyms_lookup_name("is_free_buddy_page");
    if (_is_free_buddy_page == NULL)
    {
        pr_info("lookup failed _is_free_buddy_page\n");
        return - ENXIO;
    }
    return 0;
}

int fh_do_escape(fh_ctx_t *fh_ctx, int action)
{
    unsigned long nonce = ++ fd_ctx.fh_nonce;
    fh_ctx->fh_escape_data->turn = FH_TURN_HOST;
    fh_ctx->fh_escape_data->action = action;

    if (!spin_is_locked(&fh_ctx->fh_lock)) {
        pr_info("spin lock not held on escape!\n");
        BUG();
    }

    #if defined(__x86_64__) || defined(_M_X64)
    #else
    asm volatile("dmb sy");
    #endif
    /*
     * TODO: Optimization: put data not on same page as faulthook
     */
    fh_ctx->fh_escape_data->nonce = nonce; /* escape to other world */

    // and we are back
    if (fh_ctx->fh_escape_data->turn != FH_TURN_GUEST)
    {
        pr_err("Host did not reply to request. Nonce: 0x%lx. Is host listening?", nonce);
        return - ENXIO;
    }

    // TODO: add return state here
    return 0;
}


int fh_init(fh_ctx_t **ret_fh_ctx,
            void *escape_ptr,
            unsigned long escape_size)
{
    int ret = 0;
    fh_ctx_t *fh_ctx = kmalloc(sizeof(fh_ctx_t), GFP_KERNEL);
    if (fh_ctx == NULL)
    {
        return - ENOMEM;
    }
    memset(fh_ctx, 0, sizeof(fh_ctx_t));
    ret = setup_lookup();
    if (ret < 0)
    {
        kfree(fh_ctx);
        return ret;
    }
    fh_ctx->escape_ptr = escape_ptr;
    fh_ctx->escape_size = escape_size;
    fh_ctx->fh_escape_data = (struct faultdata_struct *) escape_ptr;
    fh_ctx->fh_lock = __SPIN_LOCK_UNLOCKED(fh_lock);
    *ret_fh_ctx = fh_ctx;

    memset(escape_ptr, 0, escape_size);
    pr_info("faulthook page: 0x%lx+0x%lx bytes, pfn=%lx\n",
            (unsigned long) escape_ptr, escape_size,
                    page_to_pfn(virt_to_page((unsigned long)escape_ptr)));


    if (escape_size == 0)
    {
        pr_info("invalid escape_size: %ld\n", escape_size);
        return - EINVAL;
    }
    unsigned long pfn = page_to_pfn(virt_to_page(escape_ptr));
    struct page *epage = pfn_to_page(pfn);
    unsigned long *p = kmap(epage);
    pr_info("deref test %lx", (unsigned long) p);
    pr_info("%lx=%lx\n", (unsigned long) p, *p);

    fd_data_lock(fh_ctx);
    fh_do_escape(fh_ctx, FH_ACTION_SETUP);
    fd_data_unlock(fh_ctx);

    // TODO: BEAN
    // fh_verify_mapping
    setup_lookup();

    return 0;
}

int fh_cleanup(fh_ctx_t *fh_ctx)
{
    fd_data_lock(fh_ctx);
    fh_do_escape(fh_ctx, FH_ACTION_TEARDOWN);
    memset(fh_ctx->escape_ptr, 0, fh_ctx->escape_size);
    fd_data_unlock(fh_ctx);
    kfree(fh_ctx);
    return 0;
}

int fh_fop_open(fh_ctx_t *fh_ctx,
                struct inode *inode,
                struct file *file,
                void *private_data)
{
    char buf[256];
    char* dev_name;
    int ret;
    struct fh_fop_data *info = kmalloc(sizeof(struct fh_fop_data), GFP_KERNEL);
    struct fh_action_open *escape;
    if (info == NULL)
    {
        return - ENOMEM;
    }
    fd_data_lock(fh_ctx);
    escape = (struct fh_action_open *) &fh_ctx->fh_escape_data->data;
    info->private_data = private_data;
    file->private_data = info;
    dev_name = d_path(&file->f_path, buf, sizeof(buf));
    strncpy(escape->device, dev_name, sizeof(buf));
    escape->flags = file->f_flags;
    ret = fh_do_escape(fh_ctx, FH_ACTION_OPEN_DEVICE);
    if (ret < 0)
    {
        pr_info("fh_do_escape(FH_ACTION_OPEN_DEVICE) failed\n");
        goto clean_up;
    }
    if (escape->common.ret < 0)
    {
        ret = escape->common.err_no;
        pr_info("escape->common.ret < 0: %d\n", ret);
        goto clean_up;
    }
    /*
     * We use fd as key to query device on host
     */
    info->fd = escape->common.fd;
    ret = 0;
    clean_up:
    fd_data_unlock(fh_ctx);
    return ret;
}

/*
 * Called when the device goes from used to unused.
 */
int fh_fop_close(fh_ctx_t *fh_ctx,
                 struct inode *dev_node,
                 struct file *filep)
{
    int ret = 0;
    struct fh_fop_data *fop_data = filep->private_data;
    struct fh_action_open *escape = (struct fh_action_open *) fh_ctx->fh_escape_data->data;
    fd_data_lock(fh_ctx);
    escape->common.fd = fop_data->fd;
    ret = fh_do_escape(fh_ctx, FH_ACTION_CLOSE_DEVICE);
    if (ret < 0)
    {
        goto clean_up;
    }
    if (escape->common.ret < 0)
    {
        ret = escape->common.err_no;
        goto clean_up;
    }
    ret = 0;
    clean_up:
    fd_data_unlock(fh_ctx);
    kfree(filep->private_data);
    return ret;
}

#if 0
static int fh_verify_mapping(void)
{
    int ret = 0;
    unsigned long i, pfn;
    int pages_offline = 0, pages_busy = 0, pages_no_mapping = 0;
    int buddy_is_free = 0;
    struct zone *zone;
    unsigned long flags = 0;

    pr_info("fvp_escape mapping fixup FH_ACTION_GET_EMPTY_MAPPINGS\n");
    fd_data_lock();
    struct action_empty_mappings *a = (struct action_empty_mappings *) &fd_data->data;
    a->last_pfn = 0;
    a->pfn_max_nr_guest = DIV_ROUND_DOWN_ULL((fvp_escape_size
            - sizeof(struct faultdata_struct) - sizeof(struct action_empty_mappings)), 8);
    do
    {
        ret = fh_do_escape(FH_ACTION_GET_EMPTY_MAPPINGS);
        if (ret < 0)
        {
            break;
        }
        if (a->common.ret < 0)
        {
            break;
        }
        /*
         * XXX: Fix up routine to ensure that we dont run in any page
         * which we don't have a mapping for on the fvp userspace manager side.
         */
        for (i = 0; i < a->pfn_nr; i ++)
        {
            pfn = a->pfn[i];
            pages_no_mapping ++;
            if (pfn_valid(pfn))
            {
                struct page *page = pfn_to_page(pfn);
                if (! PagePoisoned(page))
                {
                    zone = page_zone(pfn_to_page(pfn));
                    spin_lock_irqsave(&zone->lock, flags);
                    buddy_is_free = _is_free_buddy_page(page);
                    spin_unlock_irqrestore(&zone->lock, flags);

                    if (buddy_is_free)
                    {
                        ret = _soft_offline_page(pfn, 0);
                        if (ret != 0)
                        {
                            pr_info("_soft_offline_page failed for %lx: %d\n", pfn, ret);
                        }
                        pages_offline ++;
                    } else
                    {
                        pages_busy ++;
                        // pr_info("busy page without mapping: %lx\n", pfn);
                    }

                }
            }
        }
    } while (a->last_pfn != 0);
    pr_info("pages put soft offline=%d, pages busy=%d, pages no mapping=%d\n",
            pages_offline, pages_busy, pages_no_mapping);

    clean_up:
    fd_data_unlock();
    return 0;
}

/*
 * character device file operations for control bus (through control bridge)
 */
ssize_t fh_fop_read(struct file *fp,
                    char __user *buf,
                    size_t count,
                    loff_t *pos)
{
    long ret = 0;
    struct fh_fop_data *info = fp->private_data;

    fd_data_lock();
    struct fh_action_read *a = (struct fh_action_read *) &fd_data->data;
    a->common.fd = info->fd;
    a->offset = *pos;
    a->count = count;

    fh_do_escape(FH_ACTION_READ);
    ret = a->common.ret;
    if (ret < 0)
    {
        pr_info("faulthook failed\n");
        goto clean_up;
    }
    pr_info("Got %lx bytes from host read\n", a->buffer_size);

    ret = copy_to_user(buf, a->buffer, a->buffer_size);
    if (ret < 0)
    {
        pr_info("copy_to_user failed\n");
        goto clean_up;
    }
    *pos = a->count;
    ret = a->buffer_size;

    clean_up:
    fd_data_unlock();
    return ret;
}

ssize_t fh_char_ctrl_write(struct file *file, const char __user *buf,
                           size_t count, loff_t *pos)
{

    NOT_SUPPORTED;
    struct fh_fop_data *info = file->private_data;
    long ret = 0;

    fd_data_lock();
    struct fh_action_write *a = (struct fh_action_write *) &fd_data->data;
    a->common.fd = info->fd;
    a->offset = *pos;
    a->count = 4; /*=count;*/ // XXX: driver only writes 4 bytes

    ret = copy_to_user(a->buffer, buf, a->count);
    if (ret)
    {
        pr_info("copy to user failed\n");
        goto clean_up;
    }
    fh_do_escape(FH_ACTION_WRITE);

    ret = a->common.ret;
    if (ret < 0)
    {
        pr_info("faulthook failed\n");
        goto clean_up;
    }
    pr_info("wrote %lx bytes from host write\n", a->buffer_size);

    *pos += a->count;
    ret = a->buffer_size;
    clean_up:
    fd_data_lock();
    return ret;
}

long fh_char_ctrl_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct fh_fop_data *fh_info = filp->private_data;
    struct action_ioctl *escape = (struct action_ioctl *) &fd_data->data;
    escape->common.fd = fh_info->fd;
    escape->cmd = cmd;
    escape->arg = arg;

    NOT_SUPPORTED;
    return 0;
}

static void vm_close(struct vm_area_struct *vma)
{
    /*
     * Issue:
     * We cannot unfree kmalloc region because refcount may still be >0
     * What other ways are there to hook into release of mmap when last
     * handle to memory is closed?
     * For now we dont inform host about unmaps and leave mappings alive
     */
    struct fh_fop_data *fh_info = vma->vm_file->private_data;
    struct mmap_info *mmap_info = (struct mmap_info *) vma->vm_private_data;
    struct fh_action_unmap *escape = (struct fh_action_unmap *) &fd_data->data;
    unsigned long i;
    unsigned long pfn_start;
    int ret;

    memset(escape, 0, sizeof(struct fh_action_unmap));
    escape->common.fd = fh_info->fd;
    escape->vm_start = vma->vm_start;
    escape->vm_end = vma->vm_end;
    escape->pfn_size = mmap_info->data_size >> PAGE_SHIFT;
    pfn_start = page_to_pfn(virt_to_page((char *) mmap_info->data));
    for (i = 0; i < escape->pfn_size; i ++)
    {
        escape->pfn[i] = pfn_start + i * PAGE_SIZE;
    }
    ret = fh_do_escape(FH_ACTION_UNMAP);
    if (ret < 0)
    {
        pr_info("host not online");
        goto clean_up;
    }
    if (escape->common.ret < 0)
    {
        pr_info("escape failed: %ld\n", escape->common.ret);
        goto clean_up;
    }

    clean_up:
    if (mmap_info->data != NULL)
    {
        free_pages((unsigned long) mmap_info->data, mmap_info->order);
        mmap_info->data = NULL;
    }
    if (mmap_info != NULL)
    {
        kfree(mmap_info);
        mmap_info = NULL;
    }
}

static vm_fault_t vm_fault(struct vm_fault *vmf)
{
    return 0;
}

static void vm_open(struct vm_area_struct *vma)
{
}

static struct vm_operations_struct mmap_ops = {
        .close = vm_close,
        .fault = vm_fault,
        .open = vm_open,
};

int fh_fop_mmap(struct file *file, struct vm_area_struct *vma)
{
    int ret = 0;
    unsigned long pfn, pfn_start, i;
    ssize_t size, order = 0;
    struct fh_fop_data *fh_info = file->private_data;
    struct mmap_info *mmap_info = NULL;
    struct fh_action_mmap *escape = (struct fh_action_mmap *) &fd_data->data;

    mmap_info = kmalloc(GFP_KERNEL, sizeof(struct mmap_info));
    if (mmap_info == NULL)
    {
        return - ENOMEM;
    }
    memset(mmap_info, 0, sizeof(struct mmap_info));

    size = vma->vm_end - vma->vm_start;
    if (size <= 0)
    {
        ret = - EINVAL;
        pr_info("Invalid size: %ld\n", size);
        goto err_cleanup;
    }

    mmap_info->order = __roundup_pow_of_two(size >> PAGE_SHIFT);
    mmap_info->data = (void *) __get_free_pages(GFP_KERNEL, order);
    mmap_info->data_size = size;

    if (mmap_info->data == NULL)
    {
        ret = - ENOMEM;
        goto err_cleanup;
    }
    vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
    vma->vm_flags |= (VM_IO | VM_DONTEXPAND | VM_DONTDUMP);

    /* escape */
    memset(escape, 0, sizeof(struct fh_action_mmap));
    escape->common.fd = fh_info->fd;
    escape->vm_start = vma->vm_start;
    escape->vm_end = vma->vm_end;
    escape->vm_pgoff = vma->vm_pgoff;
    escape->vm_flags = vma->vm_flags;
    escape->vm_page_prot = (unsigned long) vma->vm_page_prot.pgprot;
    escape->pfn_size = size >> PAGE_SHIFT;

    /*
     * XXX: This assumes contiguous escape buffer.
     * Change if this no longer holds
     */
    pfn_start = page_to_pfn(virt_to_page((char *) mmap_info->data));
    for (i = 0; i < escape->pfn_size; i ++)
    {
        pfn = pfn_start + i * PAGE_SIZE;
        escape->pfn[i] = pfn;
    }

    ret = fh_do_escape(FH_ACTION_MMAP);
    if (ret < 0)
    {
        pr_info("host not online");
        goto err_cleanup_rsi;
    }
    if (escape->common.ret < 0)
    {
        pr_info("escape failed");
        ret = escape->common.err_no;
        goto err_cleanup_rsi;
    }
    ret = remap_pfn_range(vma, vma->vm_start, pfn_start, size, vma->vm_page_prot);
    if (ret)
    {
        pr_info("remap_pfn_range failed");
        goto err_cleanup_rsi;
    }
#if 0
    pr_info("map kernel 0x%px to user 0x%lx, size: 0x%lx\n",
             mmap_info->data, vma->vm_start, size);
#endif

    vma->vm_private_data = mmap_info;

    /*
     * We dont unmap for now
     */
    // vma->vm_ops = &mmap_ops;
    // vm_open(vma);

    ret = 0;
    return ret;

    err_cleanup_rsi:
    err_cleanup:
    if (mmap_info->data != NULL)
    {
        free_pages((unsigned long) mmap_info->data, mmap_info->order);
        mmap_info->data = NULL;
    }
    if (mmap_info != NULL)
    {
        kfree(mmap_info);
        mmap_info = NULL;
    }
    return ret;
}

inline unsigned long fh_get_page_count(const char __user *buf, size_t len)
{
    return (((unsigned long) buf + len + PAGE_SIZE - 1)
            - ((unsigned long) buf & PAGE_MASK)) >> PAGE_SHIFT;
}

int fh_unpin_pages(struct pin_pages_struct *pinned, int do_free, bool do_write)
{
    int i;
    if (pinned == NULL)
    {
        return 0;
    }
    if (pinned->pages != NULL)
    {
        for (i = 0; i < pinned->pages_nr; i ++)
        {
            if (pinned->pages[i])
            {
                // TODO: Only on write?
                if (do_write)
                {
                    set_page_dirty_lock(pinned->pages[i]);
                }
                put_page(pinned->pages[i]);
            } else
            {
                break;
            }
        }
        if (do_free)
        {
            vfree(pinned->pages);
        }
    }
    if (do_free)
    {
        vfree(pinned);
    }
    return 0;
}

int fh_pin_pages(const char __user *buf, size_t count,
                 struct pin_pages_struct **ret_pages)
{
    int i;
    int rv;
    unsigned long len = count;
    struct pin_pages_struct *pin_pages = NULL;
    struct page **pages = NULL;
    unsigned long pages_nr = fh_get_page_count(buf, len);
    unsigned long pin_pages_alloc_size = 0;

    if (pages_nr == 0)
    {
        pr_info("pages_nr invalid\n");
        return - EINVAL;
    }
    pr_info("pinning %ld pages \n", pages_nr);
    pin_pages_alloc_size = sizeof(struct pin_pages_struct *)
            + pages_nr * sizeof(struct page_chunk);

    // XXX: vmalloc is fine, we dont need contiguous memory for this
    pin_pages = vmalloc(pin_pages_alloc_size);
    if (! pin_pages)
    {
        pr_err("pin_pages OOM: \n");
        rv = - ENOMEM;
        goto err_out;
    }
    memset(pin_pages, 0, pin_pages_alloc_size);
    pages = vmalloc(pages_nr * sizeof(struct page *));
    if (! pages)
    {
        pr_err("pages OOM.\n");
        rv = - ENOMEM;
        goto err_out;
    }

    rv = get_user_pages_fast((unsigned long) buf,
                             pages_nr,
                             1/* write */,
                             pages);
    if (rv < 0)
    {
        pr_err("unable to pin down %u user pages, %d.\n", pages_nr, rv);
        goto err_out;
    }
    if (rv != pages_nr)
    {
        pr_err("unable to pin down all %u user pages, %d.\n", pages_nr, rv);
        rv = - EFAULT;
        goto err_out;
    }
    for (i = 1; i < pages_nr; i ++)
    {
        if (pages[i - 1] == pages[i])
        {
            pr_err("duplicate pages, %d, %d.\n", i - 1, i);
            rv = - EFAULT;
            goto err_out;
        }
    }
    for (i = 0; i < pages_nr; i ++)
    {
        unsigned long offset = offset_in_page(buf);
        unsigned long nbytes = min_t(unsigned int, PAGE_SIZE - offset, len);
        unsigned long pfn = page_to_pfn(pages[i]);
#if 0
        pr_info("pfn: %lx, %lx, %lx\n", pfn, nbytes, offset);
#endif

        pin_pages->page_chunks[i] = (struct page_chunk) {
                .addr = pfn,
                .nbytes = nbytes,
                .offset = offset
        };
        flush_dcache_page(pages[i]);
        buf += nbytes;
        len -= nbytes;
    }
    if (len)
    {
        pr_err("Invalid user buffer length. Cannot map to sgl\n");
        rv = - EINVAL;
        goto err_out;
    }
    pin_pages->pages_nr = pages_nr;
    pin_pages->user_buf = (char *) buf;
    pin_pages->len = count;
    pin_pages->pages = pages;
    *ret_pages = pin_pages;

    return 0;

    err_out:
    fh_unpin_pages(pin_pages, 1, 1);
    return rv;
}
#endif
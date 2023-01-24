#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>

static int do_init(void)
{
    pr_info("hello init\n");
    #if 0
    dir = debugfs_create_dir("armcca", 0);
    if (!dir) {
        // Abort module load.
        pr_err("failed to allocate debugfs\n");
        return -1;
    }
    struct dentry *entry = 0;
    entry = debugfs_create_file(
            "test",
            0222,
            dir,
            NULL,
            &debugfs_fops);
    if (!entry) {
        pr_err("failed to create entry\n");
        return -1;
    }

    volatile char *ptr = (char *) kmalloc(4096, GFP_KERNEL);
    if (!ptr) {
        return 1;
    }
    pr_info("Allocated magic page at addr: "
    PTR_FMT
    "\n", ptr);

    memset((char *) ptr, 0, 4096);
    data = (struct data_t *) ptr;
    data->magic = 0xAABBCCDDEEFF9988;
    asm volatile("dmb sy");

    flush_cache_all();
    kthread = kthread_create(work_func, NULL, "mykthread");
    wake_up_process(kthread);
    #endif
    return 0;
}

static void do_exit(void)
{
    pr_info("module exit2\n");
    debugfs_remove_recursive(dir);
}

module_init(do_init);

module_exit(do_exit);
MODULE_LICENSE("GPL");

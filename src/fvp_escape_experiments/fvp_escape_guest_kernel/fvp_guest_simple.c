#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>

// #include <linux/dma-direct.h>
// #include <linux/dma-map-ops.h>

#define PTR_FMT "0x%llx"
#define DATA_SIZE 4032

struct __attribute__((__packed__))  data_t
{
    volatile unsigned long magic;
    volatile unsigned long turn;
    char padding[48];
    char data[DATA_SIZE];
};

struct data_t *data;


static struct dentry *dir = 0;
static bool status = 0;

#include <linux/delay.h> /* usleep_range */
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

static struct task_struct *kthread;

static int work_func(void *d)
{
    int i = 0;
    while (!kthread_should_stop())
    {
        printk(KERN_INFO
        "%d\n", i);

        pr_info("before write %ld\n", i);

        *((volatile size_t *) data->data) = i;

        // faulthook
        data->turn = i;

        asm volatile("dmb sy");
        pr_info("after write %ld \n", i);

        if (i % 10 == 0)
        {
            usleep_range(10000000, 10000001);
        }
        i++;
    }
    return 0;
}

static int debugfs_hook(void *d, u64 value)
{

    int i = 0;
    pr_info("before write %ld\n", i);
    data->magic = i;
    asm volatile("dmb sy");
    flush_cache_all();
    pr_info("after write %ld\n", i);

    return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debugfs_fops, NULL, debugfs_hook,
"%llu\n");

static int myinit(void)
{
    pr_info("hello init\n");
    dir = debugfs_create_dir("armcca", 0);
    if (!dir)
    {
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
    if (!entry)
    {
        pr_err("failed to create entry\n");
        return -1;
    }

    volatile char *ptr = (char *) kmalloc(4096, GFP_KERNEL);
    if (!ptr)
    {
        return 1;
    }
    pr_info("Allocated magic page at addr: " PTR_FMT "\n", ptr);

    memset((char *) ptr, 0, 4096);
    data = (struct data_t *) ptr;
    data->magic = 0xAABBCCDDEEFF9988;
    asm volatile("dmb sy");

    flush_cache_all();
    kthread = kthread_create(work_func, NULL, "mykthread");
    wake_up_process(kthread);
    return 0;
}

static void myexit(void)
{
    data->magic = 0;
    asm volatile("dmb sy");
    flush_cache_all();

    pr_info("module exit2\n");
    debugfs_remove_recursive(dir);
}

module_init(myinit)

module_exit(myexit)
MODULE_LICENSE("GPL");

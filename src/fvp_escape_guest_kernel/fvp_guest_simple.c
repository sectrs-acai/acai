#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
// #include <linux/dma-direct.h>
// #include <linux/dma-map-ops.h>

volatile char *ptr;
size_t magic = 0xAABBCCDDEEFF0011;
#define PTR_FMT "0x%llx"

static struct dentry *dir = 0;

static int debugfs_hook(void *data, u64 value)
{
    pr_err("Accessing page " PTR_FMT "\n", ptr);
    *((volatile char *) ptr + 8) = 0xFF;
    // arch_sync_dma_for_device(virt_to_phys(ptr), 4096, DMA_BIDIRECTIONAL);

    return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(debugfs_fops, NULL, debugfs_hook, "%llu\n");

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

    ptr = (char*) kmalloc(4096, GFP_KERNEL);
    if (!ptr)
    {
        return 1;
    }
    pr_info("Allocated magic page at addr: " PTR_FMT "\n", ptr);

    memset((char *) ptr, 0, 4096);
    *((size_t *) ptr) = magic;

    // arch_sync_dma_for_device(virt_to_phys(ptr), 4096, DMA_BIDIRECTIONAL);

    return 0;
}

static void myexit(void)
{
    pr_info("hello exit2\n");
    debugfs_remove_recursive(dir);
}

module_init(myinit)

module_exit(myexit)
MODULE_LICENSE("GPL");

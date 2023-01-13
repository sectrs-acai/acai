#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/aer.h>
#include "xdma_mod.h"
#include "xdma_cdev.h"
#include "version.h"

#define DRV_MODULE_NAME        "xdma_stub"
#define DRV_MODULE_DESC        "Stub for Xilinx XDMA Driver"

static char version[] =
        DRV_MODULE_DESC " " DRV_MODULE_NAME " v" DRV_MODULE_VERSION "\n";

struct xdma_pci_dev *xpdev = NULL;

static int enable_stub_devices(void)
{
    int rv = 0;
    struct xdma_dev *xdev;
    struct pci_dev *pci_dev;
    void *hndl;

    xpdev = kmalloc(sizeof(struct xdma_pci_dev), GFP_KERNEL);
    if (!xpdev) {
        return -ENOMEM;
    }
    memset(xpdev, 0, sizeof(struct xdma_pci_dev));

    xdev = kmalloc(sizeof(struct xdma_dev), GFP_KERNEL);
    if (!xdev) {
        return -ENOMEM;
    }
    memset(xdev, 0, sizeof(struct xdma_dev));
    xpdev->xdev = xdev;

    pci_dev = kmalloc(sizeof(struct pci_dev), GFP_KERNEL);
    if (!pci_dev) {
        return -ENOMEM;
    }
    memset(pci_dev, 0, sizeof(struct pci_dev));
    xpdev->pdev = pci_dev;

    rv = xpdev_create_interfaces(xpdev);
    if (rv) {
        return -ENOMEM;
    }
    return 0;
}

static void xpdev_free(void)
{
    struct xdma_dev *xdev = xpdev->xdev;
    xpdev_destroy_interfaces(xpdev);
    xdma_cdev_cleanup();
    xpdev->xdev = NULL;
    kfree(xpdev->pdev);
    kfree(xpdev);
}


struct faultdata_struct *fd_data = NULL;
static unsigned long fh_turn = 0;
static struct page *page;

// 2^3 = 8
#define PAGE_ORDER 3
static int faulthook_init(void)
{
#if 0
    const size_t len = 4 * 4096;
    // TODO use something else
    volatile char *ptr = (char *) kmalloc(len, GFP_KERNEL);
    if (!ptr) {
        return -1;
    }
#else
    const size_t len = (1 << PAGE_ORDER)  * 4096;
     page = alloc_pages(GFP_KERNEL, PAGE_ORDER);
    if (!page) {
        printk("alloc_page failed\n");
        return -ENOMEM;
    }
    volatile char *ptr = (volatile char *) page_address(page);
#endif

    pr_info("Allocated magic page at addr: " PTR_FMT "\n", ptr);

    memset((char *) ptr, 0, len);
    fd_data = (struct faultdata_struct *) ptr;
    fd_data->magic = FAULTDATA_MAGIC;
    fd_data->length = len;
    faultdata_flush(fd_data);

    fd_data->action = FH_ACTION_ALLOC_GUEST;
    int i = 0;
    int max = 10;
    do {
        pr_info("Waiting for host to connect %d/%d\n", i, max);
        fh_do_faulthook();
        usleep_range(3000000, 3000001);
    } while (fd_data->action!=FH_ACTION_GUEST_CONTINUE && i++ < max);

    if (fd_data->action!=FH_ACTION_GUEST_CONTINUE) {
        pr_err("no action from host\n");
        return -1;
    }

    return 0;
}

void fh_do_faulthook()
{
    fh_turn++;
    fd_data->turn = fh_turn;
    faultdata_flush(fd_data);
}

static int faulthook_cleanup(void)
{
    if (fd_data) {
        /*
         * We clear magic such that upon reload we dont get several hits with the same magic
         */
        fd_data->magic = 0;
        __free_pages(page, PAGE_ORDER);
        // kfree(fd_data);
    }
    return 0;
}


static int xdma_mod_init(void)
{
    int rv;
    rv = xdma_cdev_init();

    if (rv < 0) {
        return rv;
    }
    rv = enable_stub_devices();
    if (rv!=0) {
        pr_err("enable_stub_devices failed\n");
        return rv;
    }
    rv = faulthook_init();
    if (rv!=0) {
        pr_err("faulthook_init failed\n");
    }
    return 0;
}

static void xdma_mod_exit(void)
{
    xpdev_free();
    faulthook_cleanup();
}

module_init(xdma_mod_init);

module_exit(xdma_mod_exit);
MODULE_LICENSE("GPL");
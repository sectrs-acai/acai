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

static int probe_one(void)
{
    int rv = 0;
    struct xdma_dev *xdev;
    struct pci_dev *pci_dev;
    void *hndl;

    xpdev = kmalloc(sizeof(struct xdma_pci_dev), GFP_KERNEL);
    if (!xpdev)
    {
        return -ENOMEM;
    }
    memset(xpdev, 0, sizeof(struct xdma_pci_dev));

    xdev = kmalloc(sizeof(struct xdma_dev), GFP_KERNEL);
    if (!xdev)
    {
        return -ENOMEM;
    }
    memset(xdev, 0, sizeof(struct xdma_dev));
    xpdev->xdev = xdev;

    pci_dev = kmalloc(sizeof(struct pci_dev), GFP_KERNEL);
    if (!pci_dev)
    {
        return -ENOMEM;
    }
    memset(pci_dev, 0, sizeof(struct pci_dev));
    xpdev->pdev = pci_dev;

    rv = xpdev_create_interfaces(xpdev);
    if (rv)
    {
        return -ENOMEM;
    }
    return 0;
}

static void xpdev_free(void)
{
    struct xdma_dev *xdev = xpdev->xdev;

    pr_info("xpdev 0x%p, destroy_interfaces, xdev 0x%p.\n", xpdev, xdev);
    xpdev_destroy_interfaces(xpdev);
    xpdev->xdev = NULL;
    kfree(xpdev->pdev);
    kfree(xpdev);
}


static int xdma_mod_init(void)
{
    int rv;

    pr_info("%s", version);
    rv = xdma_cdev_init();

    if (rv < 0)
    {
        return rv;
    }
    probe_one();
    return 0;
}

static void xdma_mod_exit(void)
{
    xdma_cdev_cleanup();
    xpdev_free();
}

module_init(xdma_mod_init);
module_exit(xdma_mod_exit);
MODULE_LICENSE("GPL");
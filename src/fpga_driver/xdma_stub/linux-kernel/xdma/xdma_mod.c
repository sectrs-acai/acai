#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/aer.h>
#include "xdma_mod.h"
#include "xdma_cdev.h"
#include "version.h"

#define DRV_MODULE_NAME        "xdma"
#define DRV_MODULE_DESC        "Stub for Xilinx XDMA Driver"

static char version[] =
        DRV_MODULE_DESC " " DRV_MODULE_NAME " v" DRV_MODULE_VERSION "\n";

struct xdma_pci_dev *xpdev = NULL;

#define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)


static int enable_stub_devices(void)
{
    int rv = 0;
    struct xdma_dev *xdev;
    struct pci_dev *pci_dev;
    struct xdma_engine *engine;
    void *hndl;
    int i;

    /*
     * XXX We have to fake these devices because they dont exist.
     * The code below creates empty devices. More functionality
     * may need better initialization.
     */
    xpdev = kmalloc(sizeof(struct xdma_pci_dev), GFP_KERNEL);
    if (! xpdev)
    {
        return - ENOMEM;
    }
    memset(xpdev, 0, sizeof(struct xdma_pci_dev));

    xdev = kmalloc(sizeof(struct xdma_dev), GFP_KERNEL);
    if (! xdev)
    {
        return - ENOMEM;
    }
    memset(xdev, 0, sizeof(struct xdma_dev));
    xpdev->xdev = xdev;

    pci_dev = kmalloc(sizeof(struct pci_dev), GFP_KERNEL);
    if (! pci_dev)
    {
        return - ENOMEM;
    }
    memset(pci_dev, 0, sizeof(struct pci_dev));
    xpdev->pdev = pci_dev;

    /*
     * We fake 4 c2h and h2c devices
     */
    xpdev->c2h_channel_max = 4;
    xpdev->h2c_channel_max = 4;

    for (i = 0; i < xpdev->c2h_channel_max; i ++)
    {
        engine = &xpdev->xdev->engine_c2h[i];
        engine->magic = MAGIC_ENGINE;
        engine->channel = i;
    }
    for (i = 0; i < xpdev->h2c_channel_max; i ++)
    {
        engine = &xpdev->xdev->engine_h2c[i];
        engine->magic = MAGIC_ENGINE;
        engine->channel = i;
    }
    for (i = 0; i < xpdev->c2h_channel_max; i ++)
    {
        xpdev->sgdma_c2h_cdev[i].engine = &xpdev->xdev->engine_c2h[i];
    }
    for (i = 0; i < xpdev->h2c_channel_max; i ++)
    {
        xpdev->sgdma_h2c_cdev[i].engine = &xpdev->xdev->engine_h2c[i];
    }
    rv = xpdev_create_interfaces(xpdev);
    if (rv)
    {
        return - ENOMEM;
    }
    return 0;
}

static void xpdev_free(void)
{
    // struct xdma_dev *xdev = xpdev->xdev;
    xpdev_destroy_interfaces(xpdev);
    xdma_cdev_cleanup();
    xpdev->xdev = NULL;
    kfree(xpdev->pdev);
    kfree(xpdev);
}


static int xdma_mod_init(void)
{
    int rv;
    rv = xdma_cdev_init();
    if (rv < 0)
    {
        return rv;
    }
    rv = enable_stub_devices();
    if (rv != 0)
    {
        pr_err("enable_stub_devices failed\n");
        return rv;
    }
    rv = faulthook_init();
    if (rv != 0)
    {
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
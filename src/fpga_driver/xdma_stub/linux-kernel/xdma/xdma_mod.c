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

struct faultdata_driver_struct fd_ctx;
extern unsigned long *fvp_escape_page;
extern unsigned long fvp_escape_size;

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
    struct xdma_dev *xdev = xpdev->xdev;
    xpdev_destroy_interfaces(xpdev);
    xdma_cdev_cleanup();
    xpdev->xdev = NULL;
    kfree(xpdev->pdev);
    kfree(xpdev);
}


static int faulthook_init(void)
{
    pr_info("faulthook page: %lx+%lx\n", (unsigned long) fvp_escape_page, fvp_escape_size);
    memset(&fd_ctx, 0, sizeof(struct faultdata_driver_struct));
    memset(fvp_escape_page, 0, fvp_escape_size);
    fd_data = (struct faultdata_struct *) fvp_escape_page;

    fh_do_faulthook(FH_ACTION_SETUP);
    return 0;
}


int fh_do_faulthook(int action)
{
    unsigned long nonce = ++ fd_ctx.fh_nonce;
    fd_data->turn = FH_TURN_HOST;
    fd_data->action = action;

    #if defined(__x86_64__) || defined(_M_X64)
    #else
    asm volatile("dmb sy");
    #endif
    fd_data->nonce = nonce; /* escape to other world */
    if (fd_data->turn != FH_TURN_GUEST)
    {
        pr_err("Host did not reply to request. Nonce: 0x%lx. Is host listening?", nonce);
        return - ENXIO;
    }
    return 0;
}

static int faulthook_cleanup(void)
{
    fh_do_faulthook(FH_ACTION_TEARDOWN);
    memset(fvp_escape_page, 0, fvp_escape_size);
    return 0;
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
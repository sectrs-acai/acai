#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include <linux/ioctl.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/aer.h>
#include "xdma_mod.h"
#include "xdma_cdev.h"
#include "version.h"
#include <linux/module.h>
#include <linux/pci.h>

#define DRV_MODULE_NAME        "xdma"
#define DRV_MODULE_DESC        "Stub for Xilinx XDMA Driver"

static char version[] =
        DRV_MODULE_DESC " " DRV_MODULE_NAME " v" DRV_MODULE_VERSION "\n";

struct xdma_pci_dev *xpdev = NULL;

#define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)

module_param(param_escape_page, ulong, 0);
module_param(param_do_verify, int, 0);
module_param(param_escape_size, ulong, 0);

static char ids[1024] __initdata;

module_param_string(ids, ids, sizeof(ids), 0);
MODULE_PARM_DESC(ids, "Initial PCI IDs to add to the stub driver, format is "
                      "\"vendor:device[:subvendor[:subdevice[:class[:class_mask]]]]\""
                      " and multiple comma separated entries can be specified");

static int pci_stub_probe(struct pci_dev *dev, const struct pci_device_id *id)
{
    pci_info(dev, "claimed by stub\n");
    xpdev->pdev = dev;
    return 0;
}

static struct pci_driver stub_driver = {
        .name        = "xdma-pci-stub",
        .id_table    = NULL,    /* only dynamic id's */
        .probe        = pci_stub_probe,
        .driver_managed_dma = true,
};

/* copied from pci-stub.c in kernel */
static int __init pci_stub_init(void)
{
    char *p, *id;
    int rc;

    pr_info("pci_register_driver\n");
    rc = pci_register_driver(&stub_driver);
    if (rc)
        return rc;

    /* no ids passed actually */
    if (ids[0] == '\0')
        return 0;

    /* add ids specified in the module parameter */
    p = ids;
    while ((id = strsep(&p, ",")))
    {
        unsigned int vendor, device, subvendor = PCI_ANY_ID,
                subdevice = PCI_ANY_ID, class = 0, class_mask = 0;
        int fields;

        if (! strlen(id))
            continue;

        fields = sscanf(id, "%x:%x:%x:%x:%x:%x",
                        &vendor, &device, &subvendor, &subdevice,
                        &class, &class_mask);

        if (fields < 2)
        {
            pr_warn("pci-stub: invalid ID string \"%s\"\n", id);
            continue;
        }

        pr_info("pci-stub: add %04X:%04X sub=%04X:%04X cls=%08X/%08X\n",
                vendor, device, subvendor, subdevice, class, class_mask);

        rc = pci_add_dynid(&stub_driver, vendor, device,
                           subvendor, subdevice, class, class_mask, 0);
        if (rc)
            pr_warn("pci-stub: failed to add dynamic ID (%d)\n",
                    rc);
    }

    return 0;
}

static int enable_stub_devices(void)
{
    int rv = 0;
    struct xdma_dev *xdev;
    struct xdma_engine *engine;
    struct pci_dev *pci_dev_dummy;
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

    rv = pci_stub_init();
    if (rv < 0)
    {
        return - EINVAL;
    }

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
    pci_unregister_driver(&stub_driver);
}

module_init(xdma_mod_init);
module_exit(xdma_mod_exit);
MODULE_LICENSE("GPL");
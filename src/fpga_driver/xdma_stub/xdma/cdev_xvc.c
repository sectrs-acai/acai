#define pr_fmt(fmt)     KBUILD_MODNAME ":%s: " fmt, __func__

#include "xdma_cdev.h"
#include "cdev_xvc.h"

#define COMPLETION_LOOP_MAX    100

#define XVC_BAR_LENGTH_REG    0x0
#define XVC_BAR_TMS_REG        0x4
#define XVC_BAR_TDI_REG        0x8
#define XVC_BAR_TDO_REG        0xC
#define XVC_BAR_CTRL_REG    0x10}

static long xvc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    HERE;
    return 0;
}

/*
 * character device file operations for the XVC
 */
static const struct file_operations xvc_fops = {
        .owner = THIS_MODULE,
        .open = char_open,
        .release = char_close,
        .unlocked_ioctl = xvc_ioctl,
};

void cdev_xvc_init(struct xdma_cdev *xcdev)
        {
#ifdef __XVC_BAR_NUM__
    xcdev->bar = __XVC_BAR_NUM__;
#endif
#ifdef __XVC_BAR_OFFSET__
    xcdev->base = __XVC_BAR_OFFSET__;
#else
    xcdev->base = XVC_BAR_OFFSET_DFLT;
#endif
    pr_info("xcdev 0x%p, bar %u, offset 0x%lx.\n",
            xcdev, xcdev->bar, xcdev->base);
    cdev_init(&xcdev->cdev, &xvc_fops);
}

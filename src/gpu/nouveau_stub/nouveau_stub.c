#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "drm_def.h"
// https://github.com/tjohann/mydriver/tree/master/char_driver

#define print_info(fmt, ...) \
    dev_info(drv_dev, fmt, ##__VA_ARGS__)

#define NSTUB_NOT_IMPL printk("[nstub-not-impl] %s/%s: %d\n", __FILE__, __FUNCTION__, __LINE__)
#define IOCTL_SET_DATA 0x0001

static char *drv_name = "dri!renderD128";
module_param(drv_name, charp, S_IRUGO
| S_IWUSR);
MODULE_PARM_DESC(mystring,
"the /dev name of the driver");

static const char nstub_class_name[] = "nouveau_stub";

static dev_t dev_number;
static struct cdev *dev_object;
struct class *dev_class;
static struct device *drv_dev;

char data_s[] = "char_driver says hello crude world!";

struct nstub_data
{
    int count;
    unsigned long offset;
    char *data_s;
};


static ssize_t
nstub_read(struct file *instance,
           char __user

*user,
size_t count, loff_t
*offset)
{
NSTUB_NOT_IMPL;
return - 1;
}

static ssize_t
nstub_write(struct file *instance,
            const char __user

*user,
size_t count, loff_t
*offset)
{
NSTUB_NOT_IMPL;
return - 1;
}

static int
nstub_open(struct inode *dev_node, struct file *filep)
{
    NSTUB_NOT_IMPL;
    struct nstub_data *data_p = (struct nstub_data *) kmalloc(sizeof(struct nstub_data), GFP_USER);
    if (data_p == NULL)
    {
        dev_err(drv_dev, "open: kmalloc\n");
        return - ENOMEM;
    }
    memset(data_p, 0, sizeof(struct nstub_data));
    filep->private_data = (void *) data_p;
    return 0;
}

static int
nstub_close(struct inode *dev_node, struct file *filep)
{
    NSTUB_NOT_IMPL;
    if (filep->private_data)
    {
        kfree(instance->private_data);
    }
    return 0;
}

static long
nstub_ioctl(struct file *filp, unsigned int cmd, unsigned long __user arg)
{
    NSTUB_NOT_IMPL;
    struct nstub_data *file_priv = filp->private_data;
    unsigned int in_size, out_size, drv_size, ksize;
    bool is_driver_ioctl;
    unsigned int nr = DRM_IOCTL_NR(cmd);
    out_size, drv_size, ksize;
    bool is_driver_ioctl;

    is_driver_ioctl = nr >= DRM_COMMAND_BASE && nr < DRM_COMMAND_END;

    if (is_driver_ioctl) {
        /* driver ioctl */
        unsigned int index = nr - DRM_COMMAND_BASE;

        if (index >= dev->driver->num_ioctls)
            goto err_i1;
        index = array_index_nospec(index, dev->driver->num_ioctls);
        ioctl = &dev->driver->ioctls[index];
    } else {
        /* core ioctl */
        if (nr >= DRM_CORE_IOCTL_COUNT)
            goto err_i1;
        nr = array_index_nospec(nr, DRM_CORE_IOCTL_COUNT);
        ioctl = &drm_ioctls[nr];
    }

    drv_size = _IOC_SIZE(ioctl->cmd);
    out_size = in_size = _IOC_SIZE(cmd);
    if ((cmd & ioctl->cmd & IOC_IN) == 0)
        in_size = 0;
    if ((cmd & ioctl->cmd & IOC_OUT) == 0)
        out_size = 0;
    ksize = max(max(in_size, out_size), drv_size);

    DRM_DEBUG("comm=\"%s\" pid=%d, dev=0x%lx, auth=%d, %s\n",
              current->comm, task_pid_nr(current),
              (long)old_encode_dev(file_priv->minor->kdev->devt),
              file_priv->authenticated, ioctl->name);

    /* Do not trust userspace, use our own definition */
    func = ioctl->func;

    if (unlikely(!func)) {
        DRM_DEBUG("no function\n");
        retcode = -EINVAL;
        goto err_i1;
    }

    if (ksize <= sizeof(stack_kdata)) {


    drv_size = _IOC_SIZE(ioctl->cmd);
    out_size = in_size = _IOC_SIZE(cmd);

    if ((cmd & ioctl->cmd & IOC_IN) == 0) {
        in_size = 0;
    }
    if ((cmd & ioctl->cmd & IOC_OUT) == 0) {
        out_size = 0;
    }
    ksize = max(max(in_size, out_size), drv_size);

    DRM_DEBUG("comm=\"%s\" pid=%d, %s\n",
              current->comm, task_pid_nr(current),
              ioctl->name);

    /* Do not trust userspace, use our own definition */
    func = ioctl->func;

    if (unlikely(! func))
    {
        DRM_DEBUG("no function\n");
        retcode = - EINVAL;
        goto err_i1;
    }

    if (ksize <= sizeof(stack_kdata))
    {
        kdata = stack_kdata;
    } else
    {
        kdata = kmalloc(ksize, GFP_KERNEL);
        if (! kdata)
        {
            retcode = - ENOMEM;
            goto err_i1;
        }
    }

    if (copy_from_user(kdata,(void __user
    *)arg, in_size) != 0) {
        retcode = - EFAULT;
        goto err_i1;
    }

    if (ksize > in_size)
        memset(kdata + in_size, 0, ksize - in_size);

    retcode = drm_ioctl_kernel(filp, func, kdata, ioctl->flags);
    if (copy_to_user((void __user
    *)arg, kdata, out_size) != 0)
    retcode = - EFAULT;

    err_i1:
    if (! ioctl)
        DRM_DEBUG("invalid ioctl: comm=\"%s\", pid=%d, dev=0x%lx, auth=%d, cmd=0x%02x, nr=0x%02x\n",
                  current->comm, task_pid_nr(current),
                  (long) old_encode_dev(file_priv->minor->kdev->devt),
                  file_priv->authenticated, cmd, nr);

    if (kdata != stack_kdata)
        kfree(kdata);
    if (retcode)
        DRM_DEBUG("comm=\"%s\", pid=%d, ret=%d\n", current->comm,
                  task_pid_nr(current), retcode);
    return retcode;

    return - 1;
}

loff_t nstub_llseek(struct file *file, loff_t off, int s)
{
    NSTUB_NOT_IMPL;
    return - 1;
}

int nstub_mmap(struct file *file, struct vm_area_struct *vma)
{
    NSTUB_NOT_IMPL;
    return - 1;
}

__poll_t nstub_poll(struct file *file, struct poll_table_struct *pts)
{
    NSTUB_NOT_IMPL;
    return - 1;
}

#if 0
// nouveau drm driver:
static const struct file_operations
nouveau_driver_fops = {
    .owner = THIS_MODULE,
    .open = drm_open,
    .release = drm_release,
    .unlocked_ioctl = nouveau_drm_ioctl,
    .mmap = nouveau_ttm_mmap,
    .poll = drm_poll,
    .read = drm_read,
#if defined(CONFIG_COMPAT)
    .compat_ioctl = nouveau_compat_ioctl,
#endif
    .llseek = noop_llseek,
};
#endif

static struct file_operations fops = {
        .owner = THIS_MODULE,
        .read = nstub_read,
        .write = nstub_write,
        .open = nstub_open,
        .unlocked_ioctl = nstub_ioctl,
        .release = nstub_close,
        .mmap = nstub_mmap,
        .llseek = nstub_llseek,
        .poll = nstub_poll
};


static int nstub_dev_uevent(const struct device *dev, struct kobj_uevent_env *env)
{
    add_uevent_var(env, "DEVMODE=%#o", 0777);
    return 0;
}

static int __init

nstub_init(void)
{
    print_info("nstub_init\n");
    print_info("registering stub: /dev/%s\n", drv_name);
    if (alloc_chrdev_region(&dev_number, 0, 1, drv_name) < 0)
    {
        return - EIO;
    }
    dev_object = cdev_alloc();
    if (dev_object == NULL)
    {
        goto free_dev_number;
    }
    dev_object->owner = THIS_MODULE;
    dev_object->ops = &fops;
    if (cdev_add(dev_object, dev_number, 1))
    {
        dev_err(drv_dev, "cdev_add\n");
        goto free_cdev;
    }

    /* add sysfs/udev entry */
    dev_class = class_create(THIS_MODULE, nstub_class_name);
    if (IS_ERR(dev_class))
    {
        dev_err(drv_dev, "class_create\n");
        goto free_cdev;
    }
    dev_class->dev_uevent = nstub_dev_uevent;
    drv_dev = device_create(dev_class, NULL, dev_number, NULL, "%s",
                            drv_name);
    if (IS_ERR(drv_dev))
    {
        dev_err(drv_dev, "device_create\n");
        goto free_class;
    }

    return 0;

    free_class:
    class_destroy(dev_class);

    free_cdev:
    kobject_put(&dev_object->kobj);

    free_dev_number:
    unregister_chrdev_region(dev_number, 1);

    return - EIO;
}

static void __exit

nstub_exit(void)
{
    print_info("nstub_exit\n");
    device_destroy(dev_class, dev_number);
    class_destroy(dev_class);

    cdev_del(dev_object);
    unregister_chrdev_region(dev_number, 1);
    return;
}

module_init(nstub_init);
module_exit(nstub_exit);

MODULE_VERSION("1.0");
MODULE_ALIAS("nouveau_stub");
MODULE_LICENSE("GPL");


#if 0

ssize_t nstub_read_iter(struct kiocb *kiocb, struct iov_iter *iov_iter)
{
    NSTUB_NOT_IMPL;
    return 0;
}

ssize_t nstub_write_iter(struct kiocb *kiocb, struct iov_iter *iov_iter)
{
    NSTUB_NOT_IMPL;
    return 0;
}
int (*iopoll)(struct kiocb *kiocb, bool spin);

int (*iterate)(struct file *, struct dir_context *);

int (*iterate_shared)(struct file *, struct dir_context *);

__poll_t (*poll)(struct file *, struct poll_table_struct *);

long (*unlocked_ioctl)(struct file *, unsigned int, unsigned long);

long (*compat_ioctl)(struct file *, unsigned int, unsigned long);

unsigned long mmap_supported_flags;

int (*open)(struct inode *, struct file *);

int (*flush)(struct file *, fl_owner_t id);

int (*release)(struct inode *, struct file *);

int (*fsync)(struct file *, loff_t, loff_t, int datasync);

int (*fasync)(int, struct file *, int);

int (*lock)(struct file *, int, struct file_lock *);

ssize_t (*sendpage)(struct file *, struct page *, int, size_t, loff_t *, int);

unsigned long (*get_unmapped_area)(struct file *,
                                   unsigned long,
                                   unsigned long,
                                   unsigned long,
                                   unsigned long);

int (*check_flags)(int);

int (*flock)(struct file *, int, struct file_lock *);

ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);

ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);

int (*setlease)(struct file *, long, struct file_lock **, void **);

long (*fallocate)(struct file *file, int mode, loff_t offset,
                  loff_t len);

void (*show_fdinfo)(struct seq_file *m, struct file *f);


unsigned (*mmap_capabilities)(struct file *);
ssize_t (*copy_file_range)(struct file *, loff_t, struct file *,
                           loff_t, size_t, unsigned int);

loff_t (*remap_file_range)(struct file *file_in, loff_t pos_in,
                           struct file *file_out, loff_t pos_out,
                           loff_t len, unsigned int remap_flags);

int (*fadvise)(struct file *, loff_t, loff_t, int);
#endif


/* 
 * Some Copyright stuff was here ....
*/
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/ioctl.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>     //kmalloc()
#include <linux/uaccess.h>  //copy_to/from_user()

#define WR_VALUE_32 _IOW('a', 'a', uint32_t *)
#define WR_VALUE_16 _IOW('a', 'b', uint16_t *)
#define WR_VALUE_8 _IOW('a', 'c', uint8_t *)

#define RD_VALUE_32 _IOWR('a', 'd', uint32_t *)
#define RD_VALUE_16 _IOWR('a', 'e', uint16_t *)
#define RD_VALUE_8 _IOWR('a', 'f', uint8_t *)

struct ioctl_argument {
    uint64_t offset;
    uint64_t data;
};
struct ioctl_argument value;

dev_t dev = 0;
static struct class *dev_class;
static struct cdev etx_cdev;

static void *config_space_ptr;

/*
** Function Prototypes
*/
static int __init etx_driver_init(void);
static void __exit etx_driver_exit(void);
static int etx_open(struct inode *inode, struct file *file);
static int etx_release(struct inode *inode, struct file *file);
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off);
static ssize_t etx_write(struct file *filp, const char *buf, size_t len, loff_t *off);
static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg);

/*
** File operation sturcture
*/
static struct file_operations fops =
    {
        .owner = THIS_MODULE,
        .read = etx_read,
        .write = etx_write,
        .open = etx_open,
        .unlocked_ioctl = etx_ioctl,
        .release = etx_release,
};

/*
** This function will be called when we open the Device file
*/
static int etx_open(struct inode *inode, struct file *file) {
    pr_info("Device File Opened...!!!\n");
    return 0;
}

/*
** This function will be called when we close the Device file
*/
static int etx_release(struct inode *inode, struct file *file) {
    pr_info("Device File Closed...!!!\n");
    return 0;
}

/*
** This function will be called when we read the Device file
*/
static ssize_t etx_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    pr_info("Read Function\n");
    return 0;
}

/*
** This function will be called when we write the Device file
*/
static ssize_t etx_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    pr_info("Write function\n");
    return len;
}

/*
** This function will be called when we write IOCTL on the Device file
*/
static long etx_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
        case WR_VALUE_32:
            if (copy_from_user(&value, (struct ioctl_argument *)arg, sizeof(value))) {
                pr_err("Data Write : Err!\n");
            }
            if (value.data != (uint32_t)value.data) {
                pr_err("Int overflow while writing");
            }
            *((uint32_t *)(config_space_ptr + value.offset)) = (uint32_t)value.data;
            pr_info("WRITE: Value = %llx | Offset = %llx\n", value.data, value.offset);
            break;
        case WR_VALUE_16:
            if (copy_from_user(&value, (struct ioctl_argument *)arg, sizeof(value))) {
                pr_err("Data Write : Err!\n");
            }
            if (value.data != (uint16_t)value.data) {
                pr_err("Int overflow while writing");
            }
            *((uint16_t *)(config_space_ptr + value.offset)) = (uint16_t)value.data;
            pr_info("WRITE: Value = %llx | Offset = %llx\n", value.data, value.offset);
            break;
        case WR_VALUE_8:
            if (copy_from_user(&value, (struct ioctl_argument *)arg, sizeof(value))) {
                pr_err("Data Write : Err!\n");
            }
            if (value.data != (uint8_t)value.data) {
                pr_err("Int overflow while writing");
            }
            *((uint8_t *)(config_space_ptr + value.offset)) = (uint8_t)value.data;
            pr_info("WRITE: Value = %llx | Offset = %llx\n", value.data, value.offset);
            break;
        case RD_VALUE_32:
            if (copy_from_user(&value, (struct ioctl_argument *)arg, sizeof(value))) {
                pr_err("Data Read : Err!\n");
            }
            value.data = *((uint32_t *)(config_space_ptr + value.offset));
            pr_info("READ: Value = %llx | Offset = %llx\n", value.data, value.offset);
            if (copy_to_user((struct ioctl_argument *)arg, &value, sizeof(value))){
                pr_err("copy to user");
            }
            break;
        case RD_VALUE_16:
            if (copy_from_user(&value, (struct ioctl_argument *)arg, sizeof(value))) {
                pr_err("Data Read : Err!\n");
            }
            value.data = *((uint16_t *)(config_space_ptr + value.offset));
            pr_info("READ: Value = %llx | Offset = %llx\n", value.data, value.offset);
            if (copy_to_user((struct ioctl_argument *)arg, &value, sizeof(value))){
                pr_err("copy to user");
            }
            break;
        case RD_VALUE_8:
            if (copy_from_user(&value, (struct ioctl_argument *)arg, sizeof(value))) {
                pr_err("Data Read : Err!\n");
            }
            value.data = *((uint8_t *)(config_space_ptr + value.offset));
            pr_info("READ: Value = %llx | Offset = %llx\n", value.data, value.offset);
            if (copy_to_user((struct ioctl_argument *)arg, &value, sizeof(value))){
                pr_err("copy to user");
            }
            break;
        default:
            pr_info("Default\n");
            break;
    }
    return 0;
}

/*
** Module Init function
*/
static int __init etx_driver_init(void) {
    /*Allocating Major number*/
    if ((alloc_chrdev_region(&dev, 0, 1, "etx_Dev")) < 0) {
        pr_err("Cannot allocate major number\n");
        return -1;
    }
    pr_info("Major = %d Minor = %d \n", MAJOR(dev), MINOR(dev));

    /*Creating cdev structure*/
    cdev_init(&etx_cdev, &fops);

    /*Adding character device to the system*/
    if ((cdev_add(&etx_cdev, dev, 1)) < 0) {
        pr_err("Cannot add the device to the system\n");
        goto r_class;
    }

    /*Creating struct class*/
    if (IS_ERR(dev_class = class_create(THIS_MODULE, "etx_class"))) {
        pr_err("Cannot create the struct class\n");
        goto r_class;
    }
    // TODO: replace with config space
    config_space_ptr = kzalloc(4096, GFP_KERNEL);
    if (!config_space_ptr) {
        pr_err("could not map the config space");
        goto r_class;
    }

    /*Creating device*/
    if (IS_ERR(device_create(dev_class, NULL, dev, NULL, "etx_device"))) {
        pr_err("Cannot create the Device 1\n");
        goto r_device;
    }
    pr_info("Device Driver Insert...Done!!!\n");
    return 0;

r_device:
    class_destroy(dev_class);
r_class:
    unregister_chrdev_region(dev, 1);
    return -1;
}

/*
** Module exit function
*/
static void __exit etx_driver_exit(void) {
    device_destroy(dev_class, dev);
    class_destroy(dev_class);
    cdev_del(&etx_cdev);
    unregister_chrdev_region(dev, 1);
    pr_info("Device Driver Remove...Done!!!\n");
}

module_init(etx_driver_init);
module_exit(etx_driver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("EmbeTronicX <embetronicx@gmail.com>");
MODULE_DESCRIPTION("Simple Linux device driver (IOCTL)");
MODULE_VERSION("1.5");
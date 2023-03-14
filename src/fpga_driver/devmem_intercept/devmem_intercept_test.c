#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <asm/cacheflush.h>


#include <linux/delay.h> /* usleep_range */
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/module.h>
#include <linux/highmem.h>

MODULE_LICENSE("GPL");
static int myinit(void)
{
    pr_info("simple init\n");



    return 0;
}

static void myexit(void)
{
    pr_info("simple exit\n");
}

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");


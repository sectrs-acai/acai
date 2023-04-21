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
#include <asm/rsi_cmds.h>
#include <linux/memblock.h>

/* set when running in realm world */
static int realm = 0;
module_param(realm, int, 0);

static int debug = 0;
module_param(debug, int, 0);

#if 0
static inline int devmem_delegate_mem_range(phys_addr_t addr, unsigned long num_granules) {
    int ret = 0;

    if (likely(realm))
    {
        ret = rsi_set_addr_range_dev_mem(addr, 2 /* undelegate */, num_granules);
        #if 0
        if (ret != 0)
        {
            pr_info("rsi_set_addr_dev_mem delegate failed for %lx\n", addr);
        }
        #endif
    }
    return ret;
}
#define MIN(a, b) (a < b ? a : b)
static inline int undelegate(void) {
    long i, start, end, addr, left, range, num_pages;
    const unsigned long delegate_chunk = PAGE_SIZE * 0x800; /* 128 MB */

    pr_info("undelegating: %llx-%llx\n",
            memblock_start_of_DRAM(), memblock_end_of_DRAM());

    for_each_mem_range(i, &start, &end) {
        left = end - start;
        while(left > 0) {
            range = MIN(delegate_chunk, left);
            num_pages = DIV_ROUND_DOWN_ULL(range, PAGE_SIZE);
            devmem_delegate_mem_range(start, num_pages);
            left -= range;
            start += range;
        }
    }
}
#endif

MODULE_LICENSE("GPL");
static int myinit(void)
{
    pr_info("devmem undelegate\n");
    pr_info("debug=%d\n", debug);
    pr_info("realm=%d\n", realm);

    #if 0
    undelegate();
    #endif

    return 0;
}

static void myexit(void)
{
    pr_info("devmem undelegate exit\n");
}

module_init(myinit)
module_exit(myexit)
MODULE_LICENSE("GPL");


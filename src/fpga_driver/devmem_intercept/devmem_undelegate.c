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

#include <linux/ioctl.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/version.h>
#include <linux/kprobes.h>
#include <linux/kthread.h>
#include <linux/kallsyms.h>
#include <linux/vmalloc.h>
#include <linux/types.h>
#include <linux/highmem.h>
#include <linux/scatterlist.h>
#include <linux/pci.h>
#include <linux/dma-map-ops.h>

/*
static inline unsigned long rsi_set_addr_range_dev_mem(phys_addr_t start,
                                                       unsigned long granule_num,
                                                       enum dev_mem_state state)
*/

#define DEVMEM_DELEGATE 1
#define DEVMEM_UNDELEGATE 0

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
#define KPROBE_KALLSYMS_LOOKUP 1

typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);

kallsyms_lookup_name_t kallsyms_lookup_name_func;
#define kallsyms_lookup_name kallsyms_lookup_name_func

static struct kprobe kp = {
    .symbol_name = "kallsyms_lookup_name"
};
#endif

/* set when running in realm world */
static int realm = 1;
module_param(realm, int, 0);

static int debug = 0;
module_param(debug, int, 0);

#if 1
static inline int devmem_delegate_mem_range(phys_addr_t addr, unsigned long num_granules) {
    int ret = 0;

    if (likely(realm))
    {
        ret = rsi_set_addr_range_dev_mem(addr, num_granules,  DEVMEM_UNDELEGATE);
        #if 1
        if (ret != 0)
        {
            pr_info("rsi_set_addr_dev_mem delegate failed for %lx\n", addr);
        }
        #endif
    }
    return ret;
}


static phys_addr_t (*_memblock_start_of_DRAM) (void) = NULL;
static phys_addr_t (*_memblock_end_of_DRAM) (void) = NULL;


#define MIN(a, b) (a < b ? a : b)
static inline int undelegate(void) {
    int res = 0;
    phys_addr_t i, start, end, addr;
    long long left, range, num_pages;
    const unsigned long delegate_chunk = PAGE_SIZE * 0x800; /* 128 MB */

    pr_info("undelegating: %llx-%llx\n",
            _memblock_start_of_DRAM(), _memblock_end_of_DRAM());

    pr_info("chunk size: %lx\n", delegate_chunk);

    start = _memblock_start_of_DRAM();
    end = _memblock_end_of_DRAM();
    left = end - start;
    while(left > 0) {
        range = MIN(delegate_chunk, left);
        num_pages = DIV_ROUND_DOWN_ULL(range, PAGE_SIZE);
        devmem_delegate_mem_range(start, num_pages);
        left -= range;
        start += range;
    }
    pr_info("done\n");

    return 0;
}
#endif

MODULE_LICENSE("GPL");
static int myinit(void)
{
    pr_info("devmem undelegate\n");
    pr_info("debug=%d\n", debug);
    pr_info("realm=%d\n", realm);

    #ifdef KPROBE_KALLSYMS_LOOKUP
    register_kprobe(&kp);
    kallsyms_lookup_name = (kallsyms_lookup_name_t) kp.addr;
    unregister_kprobe(&kp);

    if (!unlikely(kallsyms_lookup_name)) {
        pr_alert("Could not retrieve kallsyms_lookup_name address\n");
        return -ENXIO;
    }
    #endif

    _memblock_start_of_DRAM = (void *) kallsyms_lookup_name("memblock_start_of_DRAM");
    if (_memblock_start_of_DRAM == NULL) {
        pr_info("_memblock_start_of_DRAM failed\n");
        return -ENXIO;
    }

    _memblock_end_of_DRAM = (void *) kallsyms_lookup_name("memblock_end_of_DRAM");
    if (_memblock_end_of_DRAM == NULL) {
        pr_info("_memblock_end_of_DRAM failed\n");
        return -ENXIO;
    }

    #if 1
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


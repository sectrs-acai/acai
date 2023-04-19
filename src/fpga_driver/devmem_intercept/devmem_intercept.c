#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/rsi_cmds.h>
#include <linux/kprobes.h>
#include <linux/dma-mapping.h>

/* set when running in realm world */
static int realm = 0;
module_param(realm, int, 0);

static int debug = 0;
module_param(debug, int, 0);

/* whether to verify devmem delegate with smmu test engine */
static int testengine = 0;
module_param(testengine, int, 0);

#define debug_print(fmt, ...) if(debug) pr_info(fmt, ##__VA_ARGS__)

/*
 * hook for:
 * int remap_pfn_range(struct vm_area_struct *, unsigned long addr,
 *                     unsigned long pfn, unsigned long size, pgprot_t);
 */
struct kprobe remap_pfn_range_probe;

/*
 * hook for:
 * unsigned int dma_map_sg_attrs(struct device *dev, struct scatterlist *sg,
 *                               int nents, enum dma_data_direction dir, unsigned long attrs);
 */
struct kprobe dma_map_sg_attrs_probe;

/* for dma_unmap_sg_attrs(d, s, n, r, 0) */
struct kprobe dma_unmap_sg_attrs_probe;

static inline int devmem_delegate_mem_device(phys_addr_t addr)
{
    int ret = 0;
    if (realm)
    {
        ret = rsi_set_addr_dev_mem(addr, 1 /* delegate */);
        if (ret != 0)
        {
            debug_print("rsi_set_addr_dev_mem delegate failed for %lx\n", addr);
        }
        if (testengine) {
            ret = rsi_trigger_testengine(addr,addr,31);
            if (ret != 0)
            {
                pr_info("rsi_trigger_testengine failed for IPA:%lx and SID: %lx\n", addr, 31);
            }
            ret = rsi_trigger_testengine(addr,addr,31);
            if (ret != 0)
            {
                pr_info("rsi_trigger_testengine failed for IPA:%lx and SID: %lx | ret %lx\n", addr, 31, ret);
            }
        }
    }
    return ret;
}

static inline int devmem_undelegate_mem_device(phys_addr_t addr)
{

    int ret = 0;
    if (realm)
    {
        ret = rsi_set_addr_dev_mem(addr, 0 /* undelegate */);
        if (ret != 0)
        {
            debug_print("rsi_set_addr_dev_mem undelegate failed for %lx\n", addr);
        }
    }
    return ret;
}

static int inline is_dev_mem(unsigned long pfn, unsigned long size)
{
    // TODO(bean): Check device tree and check if address belongs to device
    return 1;
}

static int pre_remap_pfn_range(struct kprobe *p, struct pt_regs *regs)
{
    int ret;
    unsigned long j;
    unsigned long vma = regs->regs[0];
    unsigned long addr = regs->regs[1];
    unsigned long pfn = regs->regs[2];
    unsigned long size = regs->regs[3];
    unsigned long pgprot = regs->regs[4];

    debug_print("pre_remap_pfn_range vma: %lx, addr %lx pfn %lx size %lx, pgprot %lx\n",
            vma, addr, pfn, size, pgprot);

    if (is_dev_mem(pfn << PAGE_SHIFT, size))
    {
        for(j = 0; j < size; j +=PAGE_SIZE) {
            devmem_delegate_mem_device((pfn << PAGE_SHIFT) + j);
        }
    }
    return 0;
}

static void post_remap_pfn_range(struct kprobe *p, struct pt_regs *regs,
                                 unsigned long flags)
{
    unsigned long x0 = regs->regs[0];
    unsigned long x1 = regs->regs[1];
    #if 0
    debug_print("post_remap_pfn_range: x0: %lx, x1: %lx\n", x0, x1);
    #endif
}

static int pre_dma_map_sg_attrs(struct kprobe *p, struct pt_regs *regs)
{
    int ret;
    unsigned long i, j;

    struct device *dev = (struct device *) regs->regs[0];
    struct scatterlist *sg = (struct scatterlist *) regs->regs[1];
    int nents = (int) regs->regs[2];
    enum dma_data_direction dir = (enum dma_data_direction) regs->regs[3];
    unsigned long attrs = regs->regs[4];

    debug_print("pre_dma_map_sg_attrs dev %lx, sg %lx, nents %lx, dir: %lx, attr %lx\n",
            dev, sg, nents, dir, attrs);

    for (i = 0; i < nents; i ++, sg = sg_next(sg))
    {
        #if 0
        debug_print("%d, 0x%p, pg 0x%p,%u+%u, dma 0x%llx,%u.\n",
                i,
                sg,
                sg_page(sg), sg->offset, sg->length, sg_dma_address(sg),
                sg_dma_len(sg));
        #endif
        for(j = 0; j < sg->length; j +=PAGE_SIZE) {
            devmem_delegate_mem_device(page_to_phys(sg_page(sg) + j));
        }
    }

    return 0;
}

static void post_dma_map_sg_attrs(struct kprobe *p, struct pt_regs *regs,
                                  unsigned long flags)
{
    unsigned long x0 = regs->regs[0];
    unsigned long x1 = regs->regs[1];
    #if 0
    debug_print("post_dma_map_sg_attrs: x0: %lx, x1: %lx\n", x0, x1);
    #endif
    // TODO(bean): Undelegate if call failed
}

static int pre_dma_unmap_sg_attrs(struct kprobe *p, struct pt_regs *regs)
{
    unsigned long i, j;
    struct device *dev = (struct device *) regs->regs[0];
    struct scatterlist *sg = (struct scatterlist *) regs->regs[1];
    int nents = (int) regs->regs[2];
    enum dma_data_direction dir = (enum dma_data_direction) regs->regs[3];
    unsigned long attrs = regs->regs[4];

    debug_print("pre_dma_unmap_sg_attrs dev %lx, sg %lx, nents %lx, dir: %lx, attr %lx\n",
            dev, sg, nents, dir, attrs);

    for (i = 0; i < nents; i ++, sg = sg_next(sg))
    {
        for(j = 0; j < sg->length; j +=PAGE_SIZE) {
            devmem_undelegate_mem_device(page_to_phys(sg_page(sg) + j));
        }
    }
    return 0;
}

static void post_dma_unmap_sg_attrs(struct kprobe *p, struct pt_regs *regs,
                                    unsigned long flags)
{
}


static int devmem_register_kprobes(void)
{
    int ret = 0;
    memset(&remap_pfn_range_probe, 0, sizeof(remap_pfn_range_probe));
    remap_pfn_range_probe.pre_handler = pre_remap_pfn_range;
    remap_pfn_range_probe.post_handler = post_remap_pfn_range;
    remap_pfn_range_probe.symbol_name = "remap_pfn_range";
    ret = register_kprobe(&remap_pfn_range_probe);

    if (ret != 0)
    {
        pr_err("mmap_probe kprobe failed\n");
    }
    memset(&dma_map_sg_attrs_probe, 0, sizeof(dma_map_sg_attrs_probe));
    dma_map_sg_attrs_probe.pre_handler = pre_dma_map_sg_attrs;
    dma_map_sg_attrs_probe.post_handler = post_dma_map_sg_attrs;
    dma_map_sg_attrs_probe.symbol_name = "dma_map_sg_attrs";
    ret = register_kprobe(&dma_map_sg_attrs_probe);
    if (ret != 0)
    {
        pr_err("dma_map_sg_attrs_probe kprobe failed\n");
    }
    memset(&dma_unmap_sg_attrs_probe, 0, sizeof(dma_unmap_sg_attrs_probe));
    dma_unmap_sg_attrs_probe.pre_handler = pre_dma_unmap_sg_attrs;
    dma_unmap_sg_attrs_probe.post_handler = post_dma_unmap_sg_attrs;
    dma_unmap_sg_attrs_probe.symbol_name = "dma_unmap_sg_attrs";
    ret = register_kprobe(&dma_unmap_sg_attrs_probe);
    if (ret != 0)
    {
        pr_err("dma_unmap_sg_attrs_probe kprobe failed\n");
    }
    return ret;
}

static void devmem_unregister_kprobes(void)
{
    unregister_kprobe(&remap_pfn_range_probe);
    unregister_kprobe(&dma_map_sg_attrs_probe);
    unregister_kprobe(&dma_unmap_sg_attrs_probe);
}

MODULE_LICENSE("GPL");

static int devmem_init(void)
{
    int ret;
    pr_info("monitor intercept init");
    pr_info("realm=%d\n", realm);
    pr_info("testengine verify=%d\n", testengine);

    ret = devmem_register_kprobes();
    if (ret < 0)
    {
        return ret;
    }
    pr_info("kprobes registered\n");
    return 0;
}

static void devmem_exit(void)
{
    pr_info("monitor intercept exit");
    devmem_unregister_kprobes();
}

module_init(devmem_init)

module_exit(devmem_exit)
MODULE_LICENSE("GPL");

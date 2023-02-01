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

static int enable_stub_devices(void)
{
    int rv = 0;
    struct xdma_dev *xdev;
    struct pci_dev *pci_dev;
    struct xdma_engine *engine;
    void *hndl;
    int i;

    /*
     * XXX We have to fake these devices because they dont exist
     */
    xpdev = kmalloc(sizeof(struct xdma_pci_dev), GFP_KERNEL);
    if (!xpdev) {
        return -ENOMEM;
    }
    memset(xpdev, 0, sizeof(struct xdma_pci_dev));


    xdev = kmalloc(sizeof(struct xdma_dev), GFP_KERNEL);
    if (!xdev) {
        return -ENOMEM;
    }
    memset(xdev, 0, sizeof(struct xdma_dev));
    xpdev->xdev = xdev;

    pci_dev = kmalloc(sizeof(struct pci_dev), GFP_KERNEL);
    if (!pci_dev) {
        return -ENOMEM;
    }
    memset(pci_dev, 0, sizeof(struct pci_dev));
    xpdev->pdev = pci_dev;

    xpdev->c2h_channel_max = 1;
    xpdev->h2c_channel_max = 1;

    HERE;

    for (i = 0; i < xpdev->c2h_channel_max; i++) {
        xpdev->xdev->engine_c2h[i].magic = MAGIC_ENGINE;
    }
    for (i = 0; i < xpdev->h2c_channel_max; i++) {
        xpdev->xdev->engine_h2c[i].magic = MAGIC_ENGINE;
    }
    for (i = 0; i < xpdev->c2h_channel_max; i++) {
        xpdev->sgdma_c2h_cdev[i].engine = &xpdev->xdev->engine_c2h[i];
    }
    for (i = 0; i < xpdev->h2c_channel_max; i++) {
        xpdev->sgdma_h2c_cdev[i].engine = &xpdev->xdev->engine_h2c[i];
    }

    rv = xpdev_create_interfaces(xpdev);
    if (rv) {
        return -ENOMEM;
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


// struct faultdata_struct *fd_data = NULL;
// static unsigned long fh_nonce = 0;
// static struct page *page;

struct faultdata_driver_struct fd_ctx;

// 2^3 = 8
// #define PAGE_ORDER 3
static inline void hex_dump(
        const void *data,
        size_t size
)
{
    char ascii[17];
    size_t i, j;
    ascii[16] = '\0';
    for (i = 0; i < size; ++i) {
        printk("%02X ", ((unsigned char *) data)[i]);
        if (((unsigned char *) data)[i] >= ' ' && ((unsigned char *) data)[i] <= '~') {
            ascii[i % 16] = ((unsigned char *) data)[i];
        } else {
            ascii[i % 16] = '.';
        }
        if ((i + 1) % 8==0 || i + 1==size) {
            printk(" ");
            if ((i + 1) % 16==0) {
                printk("|  %s \n", ascii);
            } else if (i + 1==size) {
                ascii[(i + 1) % 16] = '\0';
                if ((i + 1) % 16 <= 8) {
                    printk(" ");
                }
                for (j = (i + 1) % 16; j < 16; ++j) {
                    printk("   ");
                }
                printk("|  %s \n", ascii);
            }
        }
    }
}
// (0x0F000000 / 8)
#define  VM_RESERVED   (VM_DONTEXPAND | VM_DONTDUMP)
#define RAW_DATA_SIZE (256* 4096)
#define RAW_DATA_OFFSET 0x980000000UL
char *rawdataStart = NULL;


#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 7, 0)
#define KPROBE_KALLSYMS_LOOKUP 1

#include <linux/kprobes.h>

typedef unsigned long (*kallsyms_lookup_name_t)(const char *name);

kallsyms_lookup_name_t kallsyms_lookup_name_func;
#define kallsyms_lookup_name kallsyms_lookup_name_func

static struct kprobe kp = {
        .symbol_name = "kallsyms_lookup_name"
};
#endif

void (*do__flush_dcache_area)(void *addr, size_t len);

static int faulthook_init(void)
{
    #ifdef KPROBE_KALLSYMS_LOOKUP
        register_kprobe(&kp);
        kallsyms_lookup_name = (kallsyms_lookup_name_t) kp.addr;
        unregister_kprobe(&kp);

        if (!unlikely(kallsyms_lookup_name)) {
            pr_alert("Could not retrieve kallsyms_lookup_name address\n");
            return -ENXIO;
        }
    #endif

    do__flush_dcache_area = (void *) kallsyms_lookup_name("__flush_dcache_area");
    if (do__flush_dcache_area==NULL) {
        pr_info("lookup failed do__flush_dcache_area\n");
        return -ENXIO;
    }
    HERE;


    memset(&fd_ctx, 0, sizeof(struct faultdata_driver_struct));
    size_t len = (1 << FAULTDATA_PAGE_ORDER) * PAGE_SIZE;
    fd_ctx.page_order = FAULTDATA_PAGE_ORDER;


    #if USE_PAGES
//    fd_ctx.page = __get_free_pages(GFP_KERNEL, FAULTDATA_PAGE_ORDER);
//
//    if (!fd_ctx.page) {
//        printk("alloc_page failed\n");
//        return -ENOMEM;
//    }
//    volatile char *ptr = (volatile char *) page_address(fd_ctx.page);
    volatile char *ptr = (char*) __get_free_pages(GFP_KERNEL, FAULTDATA_PAGE_ORDER);
    if (!ptr) {
        printk("alloc_page failed\n");
        return -ENOMEM;
    }

            #else
        // kmalloc(len, GFP_KERNEL );
        volatile char *ptr = (char *) kmalloc(4096 * 15, GFP_KERNEL);
        if (ptr==NULL) {
            printk("kmalloc failed\n");
            return -ENOMEM;
        }
        len = 4096 * 16;
            #endif

    pr_info("allocating %lx bytes with alloc_pages: %d\n", len, USE_PAGES);

    memset((char *) ptr, 0, len);
    HERE;
    faultdata_flush(ptr);

    #if 1
    {
        size_t i;
        int j;
        pr_info("i = %d\n", len / PAGE_SIZE);
        for(i = 0; i < (len / PAGE_SIZE); i ++) {
            for(j = 0; j < PAGE_SIZE; j ++) {
                *((char* )(ptr + (i * PAGE_SIZE) + j)) = (char) i + 1;
            }
            pr_info("initing row: %d\n", i);
        }
        for(i = 0; i < ((1 << FAULTDATA_PAGE_ORDER)); i ++) {
            pr_info("i: %d,magic: %xld,  margin: %xld\n", i, ptr + i * PAGE_SIZE, ptr + (i * PAGE_SIZE) + 8);
             *((unsigned long*) (ptr + i * PAGE_SIZE)) = FAULTDATA_MAGIC;
            *((unsigned long*) (ptr + (i * PAGE_SIZE) + 8)) = i;
        }
    }
    HERE;
    do__flush_dcache_area((void*) ptr, len);


    #endif

    pr_info("Allocated magic page at addr: " PTR_FMT "\n", ptr);
    fd_data = (struct faultdata_struct *) ptr;
    fd_data->magic = FAULTDATA_MAGIC;
    fd_data->length = len;
    fd_data->action = FH_ACTION_ALLOC_GUEST;

    int i =0;
    while(1) {
        char *a = ptr + 7 * PAGE_SIZE;
        pr_info("%d\n", i);
        *a = i++;
        usleep_range(1000000, 1000001);
    }

    int i = 0, j = 0;
    int max = 20;
    do {
        pr_info("Waiting for host to connect %d/%d\n", i, max);
        fh_do_faulthook();
        usleep_range(3000000, 3000001);
    } while (fd_data->action!=FH_ACTION_GUEST_CONTINUE && i++ < max);

    if (fd_data->action!=FH_ACTION_GUEST_CONTINUE) {
        pr_err("no action from host\n");
        return 0;
    } else {
        struct action_init_guest *a = (struct action_init_guest *) &fd_data->data;
        fd_ctx.host_pg_offset = a->host_offset;
    }
    return 0;
}


void fh_do_faulthook()
{
    faultdata_flush(fd_data);
    unsigned long nonce = fd_ctx.fh_nonce++;
    pr_info("flush with nonce: %ld\n", nonce);
    fd_data->turn = FH_TURN_HOST;
    fd_data->nonce = nonce; /* faulthook */
    /*
     * TODO: Is flush needed? or do we cause several faults?
     */
    faultdata_flush(fd_data);
    if (fd_data->turn!=FH_TURN_GUEST) {
        pr_err("Host did not reply to request. Nonce: 0x%lx. Is host listening?", nonce);
    }
}

static int faulthook_cleanup(void)
{
    if (fd_data) {
        /*
         * We clear magic such that upon reload we dont get several hits with the same magic
         */
        fd_data->magic = 0;
        #if USE_PAGES
        if (fd_ctx.page)
        __free_pages(fd_ctx.page, FAULTDATA_PAGE_ORDER);
        #else
        kfree(fd_data);
        #endif
    }
    return 0;
}


static int xdma_mod_init(void)
{
    int rv;
    rv = xdma_cdev_init();

    if (rv < 0) {
        return rv;
    }
    rv = enable_stub_devices();
    if (rv!=0) {
        pr_err("enable_stub_devices failed\n");
        return rv;
    }

    #if 0
    rawdataStart = (char*) ioremap(RAW_DATA_OFFSET, RAW_DATA_SIZE);
    if (rawdataStart!=NULL){
        printk("rawdataStart at:%p  \n", rawdataStart);
        memset(rawdataStart, 'c', RAW_DATA_SIZE);
        size_t i =0;
        for(i = 0; i < RAW_DATA_SIZE; i ++) {
            if (i % 4096 == 0) {
                pr_info("%ld\n", i);
                *((unsigned long*)  rawdataStart + i) = 0x00112233112233;
            }
        }

    }else{
        printk("rawdataStart is NULL \n");
        return -1;
    }
    #endif

    rv = faulthook_init();
    if (rv!=0) {
        pr_err("faulthook_init failed\n");
    }
    return 0;
}

static void xdma_mod_exit(void)
{
    xpdev_free();
    faulthook_cleanup();
    #if 0
    if (rawdataStart != NULL) {
        printk(KERN_INFO "Unmapping memory at %p\n", rawdataStart);
        iounmap(rawdataStart);
    } else {
        printk(KERN_WARNING "No memory to unmap!\n");
    }
    #endif
}

module_init(xdma_mod_init);

module_exit(xdma_mod_exit);
MODULE_LICENSE("GPL");
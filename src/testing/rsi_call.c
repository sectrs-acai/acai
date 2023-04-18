#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/rsi_cmds.h>
#include <linux/kprobes.h>
#include <linux/dma-mapping.h>

#define SID_TO_CLAIM 31ULL

/* set when running in realm world */
static int realm = 0;


module_param(realm, int, 0);

void hexdump_memory(char *data, unsigned long byte_count) {
  for (unsigned long dumped = 0; dumped < byte_count;dumped += 16) {
    unsigned long byte_offset = dumped;
    int bytes[16];
    for (int i=0; i<16; i++) {
      bytes[i] = data[dumped + i];
    }
    char line[1000];
    char *linep = line;
    linep += sprintf(linep, "%016lx  ", byte_offset);
    for (int i=0; i<16; i++) {
      if (bytes[i] == -1) {
        linep += sprintf(linep, "?? ");
      } else {
        linep += sprintf(linep, "%02hhx ", (unsigned char)bytes[i]);
      }
    }
    linep += sprintf(linep, " |");
    for (int i=0; i<16; i++) {
      if (bytes[i] == -1) {
        *(linep++) = '?';
      } else {
        *(linep++) = bytes[i];
      }
    }
    linep += sprintf(linep, "|");
    printk("%s\n",line);
  }
}


static int devmem_init(void)
{
    int ret;
    void *page;
    void *dest_page;
    pr_info("rsi call init");
    pr_info("realm=%d\n", realm);

    page = kmalloc(4096,GFP_KERNEL);
    dest_page = kmalloc(4096,GFP_KERNEL);

    memset(page,0xBA,4096);
    memset(dest_page,0x00,4096);
    // get device with stream id 31
    pr_info("claiming device %llx",SID_TO_CLAIM);
    pr_info("claiming device %llx",SID_TO_CLAIM);
    pr_info("claiming device %llx",SID_TO_CLAIM);
    pr_info("claiming device %llx",SID_TO_CLAIM);
    ret = rsi_claim_device(SID_TO_CLAIM);
    pr_info("ret value of rsi_claim_device %x",ret);
    pr_info("ret value of rsi_claim_device %x",ret);
    pr_info("ret value of rsi_claim_device %x",ret);
    pr_info("ret value of rsi_claim_device %x",ret);
    
    pr_info("physical page (ipa) addr to move %llx",virt_to_phys(page));

    pr_info("--------------------------------------");
    pr_info("--------------------------------------");
    pr_info("--------------------------------------");
    pr_info("--------------------------------------");
    pr_info("--------------------------------------");
    pr_info("--------------------------------------");
    pr_info("--------------------------------------");
    pr_info("--------------------------------------");
    
    ret = rsi_set_addr_dev_mem(virt_to_phys(page), 1);
    pr_info("ret value of first rsi_set_addr_dev_mem: %x", ret);
    pr_info("ret value of first rsi_set_addr_dev_mem: %x", ret);
    ret = rsi_set_addr_dev_mem(virt_to_phys(dest_page), 1);
    pr_info("ret value of second rsi_set_addr_dev_mem: %x", ret);
    pr_info("ret value of second rsi_set_addr_dev_mem: %x", ret);

    // before DMA
    hexdump_memory((char*)dest_page,0x100);
    // trigger testengine from realm
    ret = rsi_trigger_testengine(virt_to_phys(page),virt_to_phys(dest_page),31);
    if (ret != 0){
      pr_err("rsi_trigger_testengine returned error 0x%x",ret);
    }
    // after DMA
    hexdump_memory((char*)dest_page,0x100);

    pr_info("access to delegated page still works");
    pr_info("access to delegated page still works");
    pr_info("access to delegated page still works");
    pr_info("access to delegated page still works");

    return 0;
}

static void devmem_exit(void)
{
    pr_info("rsi call exit (unload)");
}

MODULE_LICENSE("GPL");
module_init(devmem_init)
module_exit(devmem_exit)

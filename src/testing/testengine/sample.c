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
module_param(realm,
int, 0);

static void hexdump_memory(char *data, unsigned long byte_count)
{
    for (unsigned long dumped = 0; dumped < byte_count; dumped += 16) {
        unsigned long byte_offset = dumped;
        int bytes[16];
        for (int i = 0; i < 16; i++) {
            bytes[i] = data[dumped + i];
        }
        char line[1000];
        char *linep = line;
        linep += sprintf(linep, "%016lx  ", byte_offset);
        for (int i = 0; i < 16; i++) {
            if (bytes[i]==-1) {
                linep += sprintf(linep, "?? ");
            } else {
                linep += sprintf(linep, "%02hhx ", (unsigned char) bytes[i]);
            }
        }
        linep += sprintf(linep, " |");
        for (int i = 0; i < 16; i++) {
            if (bytes[i]==-1) {
                *(linep++) = '?';
            } else {
                *(linep++) = bytes[i];
            }
        }
        linep += sprintf(linep, "|");
        printk("%s\n", line);
    }
}


#define PAGE_VAL 0xBA
#define PAGE_TMP 0xCD


static int devmem_init(void)
{
    int ret;
    void *page;
    void *dest_page;
    void *tmp_page;
    pr_info("[i] sample init");
    pr_info("[i] realm=%d\n", realm);


    /*
     * 1. Create realm private kernel memory
     */
    pr_info("[+] allocating memory\n");
    page = kmalloc(4096, GFP_KERNEL);
    dest_page = kmalloc(4096, GFP_KERNEL);
    tmp_page = kmalloc(4096, GFP_KERNEL);
    memset(page, PAGE_VAL, 4096);
    memset(dest_page, 0x00, 4096);

    /*
     *  2. Setup test engine
     */
    pr_info("[+] setting up test engine\n");
    ret = rsi_claim_device(SID_TO_CLAIM);
    if (ret!=0) {
        pr_info("[!] claim failed. Error: %d\n", ret);
        return ret;
    }
    pr_info("[+] test engine successfully claimed\n");

    /*
     * 3. Ensure that test engine cannot access realm memory
     *    Device tries to copy memory from page to dest_page
     */
    pr_info("[+] try accessing realm private memory from test engine\n");
    ret = rsi_trigger_testengine(virt_to_phys(page), virt_to_phys(dest_page), 31);
    if (ret!=0) {
        pr_err("rsi_trigger_testengine returned error 0x%x", ret);
        return ret;
    }
    for (int i = 0; i < 4096; i++) {
        if (((char *) dest_page)[i]!=0x0) {
            pr_info("[!] failed: testengine can access data!.\n");
            return -1;
        }
    }
    pr_info("[+] Test engine cannot access realm private memory (expected)\n");


    /*
     * 4. ACAI protected mode: Delegate memory to Non-Secure protected,
     *   - Program SMMU, allow device memory access (GPC),
     *     deny core access from Non Secure (GPC)
     */
    pr_info("[+] Delegating page (ipa) addr %llx to be accessible by testengine\n",
            virt_to_phys(page));
    ret = rsi_set_addr_dev_mem(virt_to_phys(page), 1);
    if (ret!=0) {
        pr_err("[!] rsi_set_addr_dev_mem returned error 0x%x", ret);
        return ret;
    }

    ret = rsi_set_addr_dev_mem(virt_to_phys(dest_page), 1);
    if (ret!=0) {
        pr_err("[!] rsi_set_addr_dev_mem returned error 0x%x", ret);
        return ret;
    }
    pr_info("[+] Delegate success\n");

    /*
     * 5. Device can now access memory
     *    Copies memory from page to dest_page.
     */
    pr_info("[+] try accessing realm private memory from test engine (delegated) \n");

#if 1
    pr_info("[+] before access:\n");
    hexdump_memory((char *) dest_page, 0x10);
#endif

    ret = rsi_trigger_testengine(virt_to_phys(page), virt_to_phys(dest_page), 31);
    if (ret!=0) {
        pr_err("[!] rsi_trigger_testengine returned error 0x%x", ret);
    }

#if 1
    pr_info("[+] after access:\n");
    hexdump_memory((char *) dest_page, 0x10);
#endif
    for (int i = 0; i < 4096; i++) {
        if (((char *) dest_page)[i]!=((char *) page)[i]) {
            pr_info("[!] test engine access failed. data mismatch.\n");
            return -1;
        }
    }
    pr_info("[+] Successful. Memory delegated and accessible\n");

    /*
     * 6. Delegating page back to realm VM
     */
    pr_info("[+] Delegating page back to realm VM\n");
    ret = rsi_set_addr_dev_mem(virt_to_phys(page), 0);
    if (ret!=0) {
        pr_err("[!] rsi_set_addr_dev_mem returned error 0x%x", ret);
        return ret;
    }
    ret = rsi_set_addr_dev_mem(virt_to_phys(dest_page), 0);
    if (ret!=0) {
        pr_err("[!] rsi_set_addr_dev_mem returned error 0x%x", ret);
        return ret;
    }
    pr_info("[+] Done\n");

    return 0;
}

static void devmem_exit(void)
{
    pr_info("sample unload");
}

MODULE_LICENSE("GPL");
module_init(devmem_init)
module_exit(devmem_exit)

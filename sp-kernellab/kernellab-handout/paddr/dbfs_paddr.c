#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/pgtable.h>

MODULE_LICENSE("GPL");

static struct dentry *dir, *output;
static struct task_struct *task;
struct packet {
        pid_t pid;
        unsigned long vaddr;
        unsigned long paddr;
};


static ssize_t read_output(struct file *fp,
                        char __user *user_buffer,
                        size_t length,
                        loff_t *position)
{
        // Implement read file operation
        pgd_t *pgd;
        pud_t *pud;
        pmd_t *pmd;
        pte_t *pte;

        //read packet
        struct packet* user_packet = (struct packet*) user_buffer;
        pid_t pid = user_packet->pid;
        unsigned long vaddr = user_packet->vaddr;
        unsigned long paddr = 0;
        unsigned long page_num = 0;
        unsigned long page_offset = vaddr & 0xfff;

        //find process
        task = pid_task(find_vpid(pid), PIDTYPE_PID);
        
        //find page by page walk process
        pgd = task->mm->pgd;
        pud = (pud_t *)(((pgd + ((vaddr >> 39) & 0x1ff))->pgd & 0xffffffffff000) + PAGE_OFFSET);
        pmd = (pmd_t *)(((pud + ((vaddr >> 30) & 0x1ff))->pud & 0xffffffffff000) + PAGE_OFFSET);
        pte = (pte_t *)(((pmd + ((vaddr >> 21) & 0x1ff))->pmd & 0xffffffffff000) + PAGE_OFFSET);

        page_num = ((pte+((vaddr>>12)&0x1ff))->pte & 0xffffffffff000);
        page_offset = vaddr & 0xfff;
        paddr = page_num | page_offset;

        
        user_packet->paddr = paddr;

        return length;
        

}

static const struct file_operations dbfs_fops = {
        // Mapping file operations with your functions
        .read = read_output,
};

static int __init dbfs_module_init(void)
{
        // Implement init module
        dir = debugfs_create_dir("paddr", NULL);

        if (!dir) {
                printk("Cannot create paddr dir\n");
                return -1;
        }

        // Fill in the arguments below
        output = debugfs_create_file("output", S_IRWXU, dir, NULL, &dbfs_fops);

	printk("dbfs_paddr module initialize done\n");

        return 0;
}

static void __exit dbfs_module_exit(void)
{
        // Implement exit module

        debugfs_remove_recursive(dir);
	printk("dbfs_paddr module exit\n");
}

module_init(dbfs_module_init);
module_exit(dbfs_module_exit);

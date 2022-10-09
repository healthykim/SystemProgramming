#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

MODULE_LICENSE("GPL");

static struct dentry *dir, *inputdir, *ptreedir;
static struct task_struct *curr;
static char* tree;
static char* tmp;
static struct debugfs_blob_wrapper* tree_wrapper;

static ssize_t write_pid_to_input(struct file *fp, 
                                const char __user *user_buffer, 
                                size_t length, 
                                loff_t *position)
{
        pid_t input_pid;
        int i;

        sscanf(user_buffer, "%u", &input_pid);
        for(i=0; i<1000; i++){
                tmp[i] = 0;
                tree[i] = 0;
        }

        curr = pid_task(find_vpid(input_pid), PIDTYPE_PID); // Find task_struct using input_pid. Hint: pid_task
        //Tracing process tree from input_pid to init(1) process
        //Make Output Format string: process_command (process_id)

        sprintf(tree, "%s (%u)\n", curr->comm, curr->pid);
        curr = curr->real_parent;


        while(1){
                sprintf(tmp, "%s (%u)\n%s", curr->comm, curr->pid, tree);
                strcpy(tree, tmp);
                if(curr->pid == 1){
                        break;
                }
                if(curr->pid != 1)
                        curr = curr->real_parent;
                else
                        break;
        }

	tree_wrapper->data = tree;
	tree_wrapper->size = strlen(tree);

        return length;
}

static const struct file_operations dbfs_fops = {
        .write = write_pid_to_input,
};

static int __init dbfs_module_init(void)
{
        // Implement init module code

        dir = debugfs_create_dir("ptree", NULL);
        
        if (!dir) {
                printk("Cannot create ptree dir\n");
                return -1;
        }
        tmp = (char*)kmalloc(1000*sizeof(char), GFP_KERNEL);
        tree = (char*)kmalloc(1000*sizeof(char), GFP_KERNEL);
        tree_wrapper = (struct debugfs_blob_wrapper*)kmalloc(sizeof(struct debugfs_blob_wrapper), GFP_KERNEL);


        inputdir = debugfs_create_file("input", S_IWUSR, dir, NULL, &dbfs_fops);
        ptreedir = debugfs_create_blob("ptree", S_IWUSR, dir, tree_wrapper); // Find suitable debugfs API
	
	printk("dbfs_ptree module initialize done\n");

        return 0;
}

static void __exit dbfs_module_exit(void)
{
        // Implement exit module code
        debugfs_remove_recursive(dir);
	printk("dbfs_ptree module exit\n");
}

module_init(dbfs_module_init);
module_exit(dbfs_module_exit);

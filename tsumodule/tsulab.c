#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Hung");
MODULE_DESCRIPTION("TSU Lab Module");

static struct proc_dir_entry *our_proc_file = NULL;
#define PROCFS_NAME "tsulab"

static ssize_t procfile_read(struct file *file_pointer,
                             char __user *buffer,
                             size_t buffer_length,
                             loff_t *offset)
{
    char msg[] = "Hello from TSU lab module!\n";
    return simple_read_from_buffer(buffer, buffer_length, offset,
                                   msg, strlen(msg));
}

#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 6, 0)
static const struct proc_ops proc_file_ops = {
    .proc_read = procfile_read,   
};
#else
static const struct file_operations proc_file_ops = {
    .owner = THIS_MODULE,
    .read  = procfile_read,
};
#endif

static int __init tsulab_init(void)
{
    pr_info("Welcome to the Tomsk State University\n");

    our_proc_file = proc_create(PROCFS_NAME, 0444, NULL, &proc_file_ops);
    if (!our_proc_file) {
        pr_err("Error creating /proc/%s\n", PROCFS_NAME);
        return -ENOMEM;
    }
    return 0;
}

static void __exit tsulab_exit(void)
{
    if (our_proc_file) {
        proc_remove(our_proc_file);
    }
    pr_info("Tomsk State University forever!\n");
}

module_init(tsulab_init);
module_exit(tsulab_exit);

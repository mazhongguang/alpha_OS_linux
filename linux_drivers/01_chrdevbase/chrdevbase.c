#include <linux/export.h>
#include <linux/percpu.h>
#include <linux/sysctl.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/fs.h>
/*#include <linux/kernel.h>*/
/*#include <linux/io.h>*/
/*#include <linux/slab.h>*/
#include <linux/uaccess.h>

#define CHRDEVBASE_MAJOR 200
#define CHRDEVBASE_NAME "chrdevbase"

static char readbuf[100];
static char writebuf[100];
static char kerneldata[] = {"kernel data"};

static int chrdevbase_open(struct inode *inode, struct file *filp)
{
	/*printk("chrdevbase_open\r\n");*/
	return 0;
}

static int chrdevbase_release(struct inode *inode, struct file *filp)
{
	/*printk("chrdevbase_release\r\n");*/

	return 0;
}

static ssize_t chrdevbase_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	int ret = 0;
	/*printk("chrdevbase_read\r\n");*/

	memcpy(readbuf, kerneldata, sizeof(kerneldata));
	ret = copy_to_user(buf, readbuf, count);
	if (ret == 0)
	{
	}
	else
	{
	}
	return 0;
}

static ssize_t chrdevbase_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	int ret = 0;
	/*printk("chrdevbase_write\r\n");*/

	ret = copy_from_user(writebuf, buf, count);
	if (ret == 0)
	{
		printk("kernel recevdata:%s\r\n", writebuf);
	}
	else
	{
	}
	return 0;
}

static struct file_operations chrdevbase_fops = {
	.owner = THIS_MODULE,
	.open = chrdevbase_open,
	.read = chrdevbase_read,
	.write = chrdevbase_write,
	.release = chrdevbase_release,
};

static int __init chrdevbase_init(void)
{
	int ret = 0;
	printk("chrdevbase_init\r\n");
	ret = register_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME, &chrdevbase_fops);
	if (ret < 0)
	{
		printk("chrdevbase init failed!\r\n");
	}
	return 0;
}

static void __exit chrdevbase_exit(void)
{
	printk("chrdevbase_exit\r\n");
	unregister_chrdev(CHRDEVBASE_MAJOR, CHRDEVBASE_NAME);
}
/* mode entry */
module_init(chrdevbase_init);

/* mode exit */
module_exit(chrdevbase_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("MZG");


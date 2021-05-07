#include "asm-generic/int-ll64.h"
#include "asm/uaccess.h"
#include "linux/capability.h"
#include "linux/err.h"
#include "linux/gfp.h"
#include "linux/mod_devicetable.h"
#include "linux/pid.h"
#include "linux/pm.h"
#include <linux/compiler.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/printk.h>
#include <linux/timex.h>
#include <asm-generic/errno-base.h>
#include <linux/export.h>
#include <linux/percpu.h>
#include <linux/sysctl.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/slab.h>
#include "linux/property.h"
#include "linux/rcupdate.h"
#include "linux/workqueue.h"
#include "linux/gpio.h"
#include "linux/of_gpio.h"

#define TIMER_CNT 1
#define TIMER_NAME "timer"

/* timer 设备结构体 */
struct timer_dev
{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
};

/* LED */
struct timer_dev timerdev;

static int timer_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &timerdev;
	return 0;
}

static int timer_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t timer_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	struct timer_dev *dev = filp->private_data;

	return 0;
}

static ssize_t timer_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	int result = 0;
	return 0;
}

/* 操作集 */
static const struct file_operations timer_fops = {
	.owner = THIS_MODULE,
	.open = timer_open,
	.write = timer_write,
	.read = timer_read,
	.release = timer_release,
};

/* 驱动入口函数 */
static int __init timer_init(void)
{
	int result;

	timerdev.major = 0;
	if (timerdev.major)
	{
		timerdev.devid = MKDEV(timerdev.major, 0);
		result = register_chrdev_region(timerdev.devid, TIMER_CNT, TIMER_NAME);
	}
	else
	{
		result = alloc_chrdev_region(&timerdev.devid, 0, TIMER_CNT, TIMER_NAME);
		timerdev.major = MAJOR(timerdev.devid);
		timerdev.minor = MINOR(timerdev.minor);
	}
	if (result < 0)
	{
		goto fail_devid;
	}

	printk("timerdev major = %d, minor = %d \r\n", timerdev.major, timerdev.minor);

	timerdev.cdev.owner = THIS_MODULE;
	cdev_init(&timerdev.cdev, &timer_fops);

	result = cdev_add(&timerdev.cdev, timerdev.devid, TIMER_CNT);
	if (result)
	{
		goto fail_cdevadd;
	}

	timerdev.class = class_create(THIS_MODULE, TIMER_NAME);
	if (IS_ERR(timerdev.class))
	{
		result = PTR_ERR(timerdev.class);
		goto fail_class;
	}

	timerdev.device = device_create(timerdev.class, NULL, timerdev.devid, NULL, TIMER_NAME);
	if (IS_ERR(timerdev.device))
	{
		result = PTR_ERR(timerdev.device);
		goto fail_device;
	}

	return 0;

fail_device:
	class_destroy(timerdev.class);
fail_class:
	cdev_del(&timerdev.cdev);
fail_cdevadd:
	unregister_chrdev_region(timerdev.devid, TIMER_CNT);
fail_devid:
	return result;
}

/* 驱动出口函数 */
static void __exit timer_exit(void)
{
	cdev_del(&timerdev.cdev);
	unregister_chrdev_region(timerdev.devid, TIMER_CNT);

	device_destroy(timerdev.class, timerdev.devid);
	class_destroy(timerdev.class);
}

module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MZG");

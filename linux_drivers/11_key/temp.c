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

#define KEY_CNT 1
#define KEY_NAME "key"

#define KEYOFF 0
#define KEYON 1

/* key 设备结构体 */
struct key_dev
{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	int key_gpio;
};

/* LED */
struct key_dev key;

static int key_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &key;
	return 0;
}

static int key_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t key_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	int result;
	unsigned char databuf[1];
	struct key_dev *dev = filp->private_data;

	result = copy_from_user(databuf, buf, count);
	if (result < 0)
	{
		return -EFAULT;
	}

	if (databuf[0] == KEYON)
	{
		gpio_set_value(dev->key_gpio, 0);
	}
	else if (databuf[0] == KEYOFF)
	{
		gpio_set_value(dev->key_gpio, 1);
	}

	return 0;
}

static ssize_t key_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	int result = 0;
	return 0;
}

/* 操作集 */
static const struct file_operations key_fops = {
	.owner = THIS_MODULE,
	.open = key_open,
	.write = key_write,
	.read = key_read,
	.release = key_release,
};

/* 驱动入口函数 */
static int __init key_init(void)
{
	int result;

	key.major = 0;
	if (key.major)
	{
		key.devid = MKDEV(key.major, 0);
		result = register_chrdev_region(key.devid, KEY_CNT, KEY_NAME);
	}
	else
	{
		result = alloc_chrdev_region(&key.devid, 0, KEY_CNT, KEY_NAME);
		key.major = MAJOR(key.devid);
		key.minor = MINOR(key.minor);
	}
	if (result < 0)
	{
		goto fail_devid;
	}

	printk("key major = %d, minor = %d \r\n", key.major, key.minor);

	key.cdev.owner = THIS_MODULE;
	cdev_init(&key.cdev, &key_fops);

	result = cdev_add(&key.cdev, key.devid, KEY_CNT);
	if (result)
	{
		goto fail_cdevadd;
	}

	key.class = class_create(THIS_MODULE, KEY_NAME);
	if (IS_ERR(key.class))
	{
		result = PTR_ERR(key.class);
		goto fail_class;
	}

	key.device = device_create(key.class, NULL, key.devid, NULL, KEY_NAME);
	if (IS_ERR(key.device))
	{
		result = PTR_ERR(key.device);
		goto fail_device;
	}

	return 0;

fail_device:
	class_destroy(key.class);
fail_class:
	cdev_del(&key.cdev);
fail_cdevadd:
	unregister_chrdev_region(key.devid, KEY_CNT);
fail_devid:
	return result;
}

/* 驱动出口函数 */
static void __exit key_exit(void)
{
	gpio_set_value(key.key_gpio, 1);
	gpio_free(key.key_gpio);

	cdev_del(&key.cdev);
	unregister_chrdev_region(key.devid, KEY_CNT);

	device_destroy(key.class, key.devid);
	class_destroy(key.class);
}

module_init(key_init);
module_exit(key_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MZG");

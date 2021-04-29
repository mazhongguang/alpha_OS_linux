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

#define BEEP_CNT 1
#define BEEP_NAME "beep"

#define BEEPOFF 0
#define BEEPON 1

/* beep 设备结构体 */
struct beep_dev
{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	int beep_gpio;
};

/* LED */
struct beep_dev beep;

static int beep_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &beep;
	return 0;
}

static int beep_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t beep_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	int result;
	unsigned char databuf[1];
	struct beep_dev *dev = filp->private_data;

	result = copy_from_user(databuf, buf, count);
	if (result < 0)
	{
		return -EFAULT;
	}

	if (databuf[0] == BEEPON)
	{
		gpio_set_value(dev->beep_gpio, 0);
	}
	else if (databuf[0] == BEEPOFF)
	{
		gpio_set_value(dev->beep_gpio, 1);
	}

	return 0;
}

/* 操作集 */
static const struct file_operations beep_fops = {
	.owner = THIS_MODULE,
	.open = beep_open,
	.write = beep_write,
	.release = beep_release,
};

/* 驱动入口函数 */
static int __init beep_init(void)
{
	int result;

	beep.major = 0;
	if (beep.major)
	{
		beep.devid = MKDEV(beep.major, 0);
		result = register_chrdev_region(beep.devid, BEEP_CNT, BEEP_NAME);
	}
	else
	{
		result = alloc_chrdev_region(&beep.devid, 0, BEEP_CNT, BEEP_NAME);
		beep.major = MAJOR(beep.devid);
		beep.minor = MINOR(beep.minor);
	}
	if (result < 0)
	{
		goto fail_devid;
	}

	printk("beep major = %d, minor = %d \r\n", beep.major, beep.minor);

	beep.cdev.owner = THIS_MODULE;
	cdev_init(&beep.cdev, &beep_fops);

	result = cdev_add(&beep.cdev, beep.devid, BEEP_CNT);
	if (result)
	{
		goto fail_cdevadd;
	}

	beep.class = class_create(THIS_MODULE, BEEP_NAME);
	if (IS_ERR(beep.class))
	{
		result = PTR_ERR(beep.class);
		goto fail_class;
	}

	beep.device = device_create(beep.class, NULL, beep.devid, NULL, BEEP_NAME);
	if (IS_ERR(beep.device))
	{
		result = PTR_ERR(beep.device);
		goto fail_device;
	}


	beep.nd = of_find_node_by_path("/beep");
	if (beep.nd == NULL)
	{
		result = -EINVAL;
		goto fail_nd;
	}

	beep.beep_gpio = of_get_named_gpio(beep.nd, "beep-gpios", 0);
	if (beep.beep_gpio < 0)
	{
		result = -EINVAL;
		goto fail_nd;
	}

	result = gpio_request(beep.beep_gpio, "beep-gpio");
	if (result)
	{
		printk("can't request beep gpio\r\n");
		goto fail_nd;
	}

	result = gpio_direction_output(beep.beep_gpio, 0);
	if (result < 0)
	{
		goto fail_set;
	}

	gpio_set_value(beep.beep_gpio, 1);

	return 0;

fail_set:
	gpio_free(beep.beep_gpio);
fail_nd:
	device_destroy(beep.class, beep.devid);
fail_device:
	class_destroy(beep.class);
fail_class:
	cdev_del(&beep.cdev);
fail_cdevadd:
	unregister_chrdev_region(beep.devid, BEEP_CNT);
fail_devid:
	return result;
}

/* 驱动出口函数 */
static void __exit beep_exit(void)
{
	gpio_set_value(beep.beep_gpio, 1);
	gpio_free(beep.beep_gpio);

	cdev_del(&beep.cdev);
	unregister_chrdev_region(beep.devid, BEEP_CNT);

	device_destroy(beep.class, beep.devid);
	class_destroy(beep.class);
}

module_init(beep_init);
module_exit(beep_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MZG");

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

#define GPIOLED_CNT 1
#define GPIOLED_NAME "gpioled"

#define LEDOFF 0
#define LEDON 1

/* gpioled 设备结构体 */
struct gpioled_dev
{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	int led_gpio;
};

/* LED */
struct gpioled_dev gpioled;

static int led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &gpioled;
	return 0;
}

static int led_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t led_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	struct gpioled_dev *dev = filp->private_data;
	int result;
	unsigned char databuf[1];

	result = copy_from_user(databuf, buf, count);
	if (result < 0)
	{
		return -EINVAL;
	}

	if (databuf[0] == LEDON)
	{
		gpio_set_value(gpioled.led_gpio, 0);
	}
	else if (databuf[0] == LEDOFF)
	{
		gpio_set_value(gpioled.led_gpio, 1);
	}		

	return 0;
}

/* 操作集 */
static const struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.open = led_open,
	.write = led_write,
	.release = led_release,
};

/* 驱动入口函数 */
static int __init led_init(void)
{
	int result;

	gpioled.major = 0;
	if (gpioled.major)
	{
		gpioled.devid = MKDEV(gpioled.major, 0);
		register_chrdev_region(gpioled.devid, GPIOLED_CNT, GPIOLED_NAME);
	}
	else
	{
		alloc_chrdev_region(&gpioled.devid, 0, GPIOLED_CNT, GPIOLED_NAME);
		gpioled.major = MAJOR(gpioled.devid);
		gpioled.minor = MINOR(gpioled.minor);
	}

	printk("gpioled major = %d, minor = %d \r\n", gpioled.major, gpioled.minor);

	gpioled.cdev.owner = THIS_MODULE;
	cdev_init(&gpioled.cdev, &led_fops);

	result = cdev_add(&gpioled.cdev, gpioled.devid, GPIOLED_CNT);

	gpioled.class = class_create(THIS_MODULE, GPIOLED_NAME);
	if (IS_ERR(gpioled.class))
	{
		return PTR_ERR(gpioled.class);
	}

	gpioled.device = device_create(gpioled.class, NULL, gpioled.devid, NULL, GPIOLED_NAME);
	if (IS_ERR(gpioled.device))
	{
		return PTR_ERR(gpioled.device);
	}

	gpioled.nd = of_find_node_by_path("/gpioled");
	if (gpioled.nd == NULL)
	{
		result = -EINVAL;
		goto fail_findnode;
	}

	gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpios", 0);
	if (gpioled.led_gpio < 0)
	{
		printk("cant't find led gpio \r\n");
		result = -EINVAL;
		goto fail_findnode;
	}
	printk("led gpio num = %d\r\n", gpioled.led_gpio);

	result = gpio_request(gpioled.led_gpio, "led-gpio");
	if (result)
	{
		printk("failed to gpio_request the led gpio \r\n");
		result = -EINVAL;
		goto fail_findnode;
	}

	/* 使用IO */
	result = gpio_direction_output(gpioled.led_gpio, 1);
	if (result)
	{
		goto fail_setoutput;
	}

	gpio_set_value(gpioled.led_gpio, 0);

	return 0;

fail_setoutput:
	gpio_free(gpioled.led_gpio);
fail_findnode:
	return result;
}

/* 驱动出口函数 */
static void __exit led_exit(void)
{
	/* 关灯 */
	gpio_set_value(gpioled.led_gpio, 1);

	cdev_del(&gpioled.cdev);
	unregister_chrdev_region(gpioled.devid, GPIOLED_CNT);

	device_destroy(gpioled.class, gpioled.devid);
	class_destroy(gpioled.class);

	gpio_free(gpioled.led_gpio);
}

module_init(led_init);
module_exit(led_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MZG");

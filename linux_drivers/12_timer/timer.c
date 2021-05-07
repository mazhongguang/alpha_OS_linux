#include "asm-generic/int-ll64.h"
#include "asm/uaccess.h"
#include "linux/capability.h"
#include "linux/err.h"
#include "linux/gfp.h"
#include "linux/jiffies.h"
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
#include "linux/timer.h"
#include "linux/workqueue.h"
#include "linux/gpio.h"
#include "linux/of_gpio.h"

#define TIMER_CNT 1
#define TIMER_NAME "timer"

#define CLOSE_CMD _IO(0XEF, 1)
#define OPEN_CMD _IO(0XEF, 2)
#define SETPERIOD_CMD _IOW(0XEF, 3, int)

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
	struct timer_list timer; /* 定时器 */

	int timerperiod;		/* 定时周期 单位：ms */
	unsigned int led_gpio;
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

static long timer_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0, value = 0;
	struct timer_dev *dev = file->private_data;

	switch (cmd)
	{
		case CLOSE_CMD:
			del_timer_sync(&dev->timer);
			break;
		case OPEN_CMD:
			mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->timerperiod));
			break;
		case SETPERIOD_CMD:
			ret = __copy_from_user(&value, (int *)arg, sizeof(int));
			/*ret = copy_from_user(&value, (int *)arg, sizeof(int));*/
			if (ret < 0)
			{
				return -EFAULT;
			}
			dev->timerperiod = value;
			mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->timerperiod));
			break;
	}

	return ret;
}

/* 操作集 */
static const struct file_operations timer_fops = {
	.owner = THIS_MODULE,
	.open = timer_open,
	.write = timer_write,
	.read = timer_read,
	.release = timer_release,
	.unlocked_ioctl = timer_ioctl,
};

/* 定时器处理函数 */
static void timer_func(unsigned long arg)
{
	struct timer_dev *dev = (struct timer_dev *)arg;
	static int sta = 1;

	sta = !sta;
	gpio_set_value(dev->led_gpio, sta);

	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(dev->timerperiod));
}

/* 初始化LED */
static int led_init(struct timer_dev *dev)
{
	int result = 0;

	dev->nd = of_find_node_by_path("/gpioled");
	if (dev->nd == NULL)
	{
		result = -EINVAL;
		goto fail_fd;
	}

	dev->led_gpio = of_get_named_gpio(dev->nd, "led-gpios", 0);
	if (dev->led_gpio < 0)
	{
		result = -EINVAL;
		goto fail_gpio;
	}

	result = gpio_request(dev->led_gpio, "led");
	if (result)
	{
		result = -EBUSY;
		printk("IO %d can't request!\r\n", dev->led_gpio);
		goto fail_request;
	}

	result = gpio_direction_output(dev->led_gpio, 1); /* 设置输出，默认关灯 */
	if (result < 0)
	{
		result = -EINVAL;
		goto fail_gpioset;
	}

	return 0;

fail_gpioset:
	gpio_free(dev->led_gpio);
fail_request:
fail_gpio:
fail_fd:
	return result;
}

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

	result = led_init(&timerdev);
	if (result < 0)
	{
		goto fail_ledinit;
	}

	/* 初始化定时器 */
	init_timer(&timerdev.timer);
	timerdev.timerperiod = 500;
	timerdev.timer.function = timer_func;
	timerdev.timer.expires = jiffies + msecs_to_jiffies(timerdev.timerperiod);
	timerdev.timer.data = (unsigned long)&timerdev;
	add_timer(&timerdev.timer);	/* 添加到系统 */


	return 0;

fail_ledinit:
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
	gpio_set_value(timerdev.led_gpio, 1);
	gpio_free(timerdev.led_gpio);

	del_timer_sync(&timerdev.timer);

	cdev_del(&timerdev.cdev);
	unregister_chrdev_region(timerdev.devid, TIMER_CNT);

	device_destroy(timerdev.class, timerdev.devid);
	class_destroy(timerdev.class);
}

module_init(timer_init);
module_exit(timer_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MZG");

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

#define KEY0VALUE 0XF0
#define INVAKEY 0X00

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

	atomic_t keyvalue;
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

	return 0;
}

static ssize_t key_read(struct file *file, char __user *buf, size_t count, loff_t *ppos)
{
	int result = 0;
	int value;
	struct key_dev *dev = file->private_data;

	if (gpio_get_value(dev->key_gpio) == 0)		/* 按下按键 */
	{
		while (!gpio_get_value(dev->key_gpio)); /* 一直等待按键释放 */
		atomic_set(&dev->keyvalue, KEY0VALUE);
	}
	else
	{
		atomic_set(&dev->keyvalue, INVAKEY);
	}
	value = atomic_read(&dev->keyvalue);
	result = __copy_to_user(buf, &value, sizeof(value));

	return result;
}

/* 操作集 */
static const struct file_operations key_fops = {
	.owner = THIS_MODULE,
	.open = key_open,
	.write = key_write,
	.read = key_read,
	.release = key_release,
};

/* key io Init */
static int keyio_init(struct key_dev *dev)
{
	int result = 0;
	
	/* 初始化atomic */
	atomic_set(&key.keyvalue, INVAKEY);

	dev->nd = of_find_node_by_path("/key");
	if (dev->nd == NULL)
	{
		result = -EINVAL;
		goto fail_nd;
	}

	dev->key_gpio = of_get_named_gpio(dev->nd, "key-gpios", 0);
	if (dev->key_gpio < 0)
	{
		result = -EINVAL;
		goto fail_gpio;
	}
	
	result = gpio_request(dev->key_gpio, "key0");
	if (result)
	{
		result = -EBUSY;
		printk(" IO %d can't request !\r\n", dev->key_gpio);
		goto fail_request;
	}

	result = gpio_direction_input(dev->key_gpio);
	if (result < 0)
	{
		result = -EINVAL;
		goto fail_input;
	}

	return 0;

fail_input:
	gpio_free(dev->key_gpio);
fail_request:
fail_gpio:
fail_nd:
	return result;
}

/* key io deinit */
static void keyio_deinit(struct key_dev *dev)
{
	gpio_free(dev->key_gpio);
}

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

	result = keyio_init(&key);
	if (result < 0)
	{
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
	keyio_deinit(&key);

	cdev_del(&key.cdev);
	unregister_chrdev_region(key.devid, KEY_CNT);

	device_destroy(key.class, key.devid);
	class_destroy(key.class);
}

module_init(key_init);
module_exit(key_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MZG");

#include "asm-generic/int-ll64.h"
#include "linux/capability.h"
#include "linux/err.h"
#include "linux/gfp.h"
#include "linux/mod_devicetable.h"
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

#define DTSLED_CNT  1	/* 设备号个数 */
#define DTSLED_NAME  "dtsled" /* name */

#define LEDON 1
#define LEDOFF 0

/* 地址映射后的虚拟地址指针 */
 static void __iomem *IMX6U_CCM_CCGR1;
 static void __iomem *SW_MUX_GPIO1_IO03;
 static void __iomem *SW_PAD_GPIO1_IO03;
 static void __iomem *GPIO1_DR;
 static void __iomem *GPIO1_GDIR;

/* dtsled device struct */
struct dtsled_dev
{
	dev_t devid;	/* device number */
	struct cdev cdev;	/* 字符设备 */
	struct class *class; /* 类 */
	struct device *device; /* 设备 */
	int major;		/* main device number */
	int minor;
	struct device_node *nd; /* 设备节点 */
};

struct dtsled_dev dtsled;

/* LED灯打开/关闭 */
static void led_switch(uint8_t sta)
{
	uint32_t val;

	if (sta == LEDON)
	{
		val = readl(GPIO1_DR);
		val &= ~(1 << 3);
		writel(val, GPIO1_DR);
	}
	else if (sta == LEDOFF)
	{
		val = readl(GPIO1_DR);
		val |= 1 << 3;
		writel(val, GPIO1_DR);
	}
}

static int dtsled_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &dtsled;
	return 0;
}

static int dtsled_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	struct dtsled_dev *dev = (struct dtsled_dev *)filp->private_data;
	int retvalue;
	uint8_t databuf[1];

	retvalue = copy_from_user(databuf, buf, count);
	if (retvalue < 0)
	{
		return -EFAULT;
	}

	/* 判断是开灯还是关灯 */
	led_switch(databuf[0]);

	return 0;
}

static int dtsled_release(struct inode *inode, struct file *filp)
{
	struct dtsled_dev *dev = (struct dtsled_dev *)filp->private_data;
	return 0;
}

/* 字符设备操作集 */
static const struct file_operations dtsled_fops = {
	.owner = THIS_MODULE,
	.write = dtsled_write,
	.open = dtsled_open,
	.release = dtsled_release,
};

/* module entry */
static int __init dtsled_init(void)
{
	int result = 0;
	const char *str;
	uint32_t regdata[10];
	u8 i = 0;
	uint32_t val;
	
	/* register char device */
	dtsled.major = 0;
	if (dtsled.major)
	{
		dtsled.devid = MKDEV(dtsled.major, 0);
		result = register_chrdev_region(dtsled.devid, DTSLED_CNT, DTSLED_NAME);
	}
	else
	{
		result = alloc_chrdev_region(&dtsled.devid, 0, DTSLED_CNT, DTSLED_NAME);
		dtsled.major = MAJOR(dtsled.devid);
		dtsled.minor = MINOR(dtsled.devid);
	}
	if (result < 0)
	{
		goto fail_devid;
	}
	
	/* 2,添加字符设备 */
	dtsled.cdev.owner = THIS_MODULE;
	cdev_init(&dtsled.cdev, &dtsled_fops);
	result = cdev_add(&dtsled.cdev, dtsled.devid, DTSLED_CNT);
	if (result < 0)
	{
		goto fail_cdev;
	}

	/* 3, 自动创建设备节点 */
	dtsled.class = class_create(THIS_MODULE, DTSLED_NAME);
	if (IS_ERR(dtsled.class))
	{
		result = PTR_ERR(dtsled.class);
		goto fail_class;
	}

	dtsled.device = device_create(dtsled.class, NULL, dtsled.devid, NULL, DTSLED_NAME);
	if (IS_ERR(dtsled.device))
	{
		result = PTR_ERR(dtsled.device);
		goto fail_device;
	}

	/* 获取设备树属性内容 */
	dtsled.nd = of_find_node_by_path("/alphaled");
	if (dtsled.nd == NULL)
	{
		result = -EINVAL;
		goto fail_findnd;
	}

	/* 获取属性 */
	result = of_property_read_string(dtsled.nd, "status", &str);
	if (result < 0)
	{
		goto fail_rs;
	}
	else
	{
		printk("status = %s \r\n", str);
	}

	result = of_property_read_string(dtsled.nd, "compatible", &str);
	if (result < 0)
	{
		goto fail_rs;
	}
	else
	{
		printk("compatible = %s \r\n", str);
	}

#if 0
	result = of_property_read_u32_array(dtsled.nd, "reg", regdata, 10);
	if (result < 0)
	{
		goto fail_rs;
	}
	else
	{
		printk("reg data:\r\n");
		for (i = 0; i < 10; i++)
		{
			printk("%#x ", regdata[i]);
		}
		printk("\r\n");
	}

	/* LED灯初始化 */
	/* 初始化LED灯，地址映射 */
	IMX6U_CCM_CCGR1 = ioremap(regdata[0], regdata[1]);
	SW_MUX_GPIO1_IO03 = ioremap(regdata[2], regdata[3]);
	SW_PAD_GPIO1_IO03 = ioremap(regdata[4], regdata[5]);
	GPIO1_DR = ioremap(regdata[6], regdata[7]);
	GPIO1_GDIR = ioremap(regdata[8], regdata[9]);
#else
	IMX6U_CCM_CCGR1 = of_iomap(dtsled.nd, 0);
	SW_MUX_GPIO1_IO03 = of_iomap(dtsled.nd, 1);
	SW_PAD_GPIO1_IO03 = of_iomap(dtsled.nd, 2);
	GPIO1_DR = of_iomap(dtsled.nd, 3);
	GPIO1_GDIR = of_iomap(dtsled.nd, 4);
#endif

	/* 初始化 */
	val = readl(IMX6U_CCM_CCGR1);
	val &= ~(3 << 26);
	val |= 3 << 26;
	writel(val, IMX6U_CCM_CCGR1);

	writel(0x5, SW_MUX_GPIO1_IO03);
	writel(0x10B0, SW_PAD_GPIO1_IO03);

	val = readl(GPIO1_GDIR);
	val |= 1 << 3;
	writel(val, GPIO1_GDIR);

	val = readl(GPIO1_DR);
	val |= 1 << 3;
	writel(val, GPIO1_DR);
	
	return 0;
	
fail_rs:
fail_findnd:
	device_destroy(dtsled.class, dtsled.devid);
fail_device:
	class_destroy(dtsled.class);
fail_class:
	cdev_del(&dtsled.cdev);
fail_cdev:
	unregister_chrdev_region(dtsled.devid, DTSLED_CNT);
fail_devid:
	return result;
}

/* module exit */
static void __exit dtsled_exit(void)
{
	uint32_t val = 0;

	val = readl(GPIO1_DR);
	val |= 1 << 3;
	writel(val, GPIO1_DR);

	/* 取消地址映射 */
	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_DR);
	iounmap(GPIO1_GDIR);

	/* 删除字符设备 */
	cdev_del(&dtsled.cdev);

	/* 释放设备号 */
	unregister_chrdev_region(dtsled.devid, DTSLED_CNT);

	/* 摧毁设备 */
	device_destroy(dtsled.class, dtsled.devid);

	/* 摧毁类 */
	class_destroy(dtsled.class);
}

/* module entry and exit */
module_init(dtsled_init);
module_exit(dtsled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MZG");

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


#define NEWCHRLED_NAME "newchrled"
#define NEWCHRLED_COUNT 1

/* 寄存器物理地址 */
#define CCM_CCGR1_BASE (0x020C406C)
#define SW_MUX_GPIO1_IO03_BASE (0x020E0068)
#define SW_PAD_GPIO1_IO03_BASE (0x020E02F4)
#define GPIO1_DR_BASE (0x0209C000)
#define GPIO1_GDIR_BASE (0x0209C004)

/* 地址映射后的虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

#define LEDOFF 0
#define LEDON 1

/* @brief LED ON/OFF
 * @param
 * @reval none
 */
static void led_switch(uint8_t sta)
{
	uint32_t val = 0;
	if (sta == LEDON)
	{
		val = readl(GPIO1_DR);
		/* bit3 clear , turn on led */
		val &= ~(1 << 3);
		writel(val, GPIO1_DR);
	}
	else if (sta == LEDOFF)
	{
		val = readl(GPIO1_DR);
		/* bit3 set, turn off led */
		val |= 1 << 3;
		writel(val, GPIO1_DR);
	}
}

/* LED设备结构体 */
struct newchrled_dev 
{
	struct cdev cdev; /* 字符设备 */
	dev_t devid;	/* 设备号 */
	int major;		/* 主设备号 */
	int minor;		/* 次设备号 */
};

struct newchrled_dev newchrled; /* led device */

static int newchrled_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int newchrled_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t newchrled_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	unsigned int retvalue;
	unsigned char databuf[1];

	retvalue = copy_from_user(databuf, buf, count);
	if (retvalue < 0)
	{
		printk("kernel write failed!\r\n");
		return -EFAULT;
	}

	led_switch(databuf[0]);

	return 0;
}

static const struct file_operations newchrled_fops = 
{
	.owner = THIS_MODULE,
	.write = newchrled_write,
	.open = newchrled_open,
	.release = newchrled_release,
};

/* entry */
static int __init newchrled_init(void)
{
	int ret = 0;
	unsigned int val = 0;

	/* 1.初始化LED，地址映射 */
	IMX6U_CCM_CCGR1 = ioremap(CCM_CCGR1_BASE, 4);
	SW_MUX_GPIO1_IO03 = ioremap(SW_MUX_GPIO1_IO03_BASE, 4);
	SW_PAD_GPIO1_IO03 = ioremap(SW_PAD_GPIO1_IO03_BASE, 4);
	GPIO1_DR = ioremap(GPIO1_DR_BASE, 4);
	GPIO1_GDIR = ioremap(GPIO1_GDIR_BASE, 4);

	/* 2.初始化 */
	val = readl(IMX6U_CCM_CCGR1);
	val &= ~(3 << 26);
	val |= 3 << 26;
	writel(val, IMX6U_CCM_CCGR1);

	/* 设置复用 */
	writel(0x5, SW_MUX_GPIO1_IO03);	

	/* 设置电气属性 */
	writel(0x10B0, SW_PAD_GPIO1_IO03);

	val = readl(GPIO1_GDIR);
	val |= 1 << 3;
	writel(val, GPIO1_GDIR);

	val = readl(GPIO1_DR);
	/* bit3 set, turn off led */
	val |= 1 << 3;
	writel(val, GPIO1_DR);

	/* 2. register char device */
	if (newchrled.major)
	{
		newchrled.devid = MKDEV(newchrled.major, 0);
		ret = register_chrdev_region(newchrled.devid, NEWCHRLED_COUNT, NEWCHRLED_NAME);
	}
	else
	{
		ret =alloc_chrdev_region(&newchrled.devid, 0, NEWCHRLED_COUNT, NEWCHRLED_NAME);
		newchrled.major = MAJOR(newchrled.devid);
		newchrled.minor = MINOR(newchrled.devid);
	}

	if (ret < 0)
	{
		printk("newchrled chrdev_region err!\r\n");
		return -1;
	}
	printk("newchrled major=%d, minor=%d\r\n", newchrled.major, newchrled.minor);

	/* 3. register char device */
	newchrled.cdev.owner = THIS_MODULE;
	cdev_init(&newchrled.cdev, &newchrled_fops);
	ret = cdev_add(&newchrled.cdev, newchrled.devid, NEWCHRLED_COUNT);

	return 0;
}

/* exit */
static void __exit newchrled_exit(void)
{
	unsigned int val = 0;
	val = readl(GPIO1_DR);
	/* bit3 set, turn off led */
	val |= 1 << 3;
	writel(val, GPIO1_DR);

	/* 1.取消地址映射 */
	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_DR);
	iounmap(GPIO1_GDIR);

	/* delete char device */
	cdev_del(&newchrled.cdev);

	/* unregister device */
	unregister_chrdev_region(newchrled.devid, NEWCHRLED_COUNT);
}

/* 注册和卸载驱动 */
module_init(newchrled_init);
module_exit(newchrled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MZG");


#include "asm-generic/int-ll64.h"
#include "asm/uaccess.h"
#include "linux/capability.h"
#include "linux/err.h"
#include "linux/errno.h"
#include "linux/gfp.h"
#include "linux/irqreturn.h"
#include "linux/jiffies.h"
#include "linux/kernel.h"
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
#include "linux/sched.h"
#include "linux/signal.h"
#include "linux/timer.h"
#include "linux/wait.h"
#include "linux/workqueue.h"
#include "linux/gpio.h"
#include "linux/of_gpio.h"
#include <linux/irq.h>
#include <linux/of_irq.h>
#include <asm/mach/map.h>
#include <linux/interrupt.h>
#include "asm/atomic.h"
#include <linux/ide.h>

#define IRQ_CNT 1
#define IRQ_NAME "irq" 
#define KEY0VALUE 0X01
#define INVAKEY 0XFF
#define KEY_NUM 1

/* 中断IO描述结构体*/
struct irq_keydesc
{
	int gpio;				/* GPIO */
	int irqnum;				/* 中断号 */
	unsigned char value;	/* 按键对应的键值 */
	char name[10];			/* 名字 */
	irqreturn_t (*handler)(int, void *);	/* 中断服务函数 */
};

/* irq 设备结构体 */
struct irq_dev
{
	dev_t devid;
	int major;
	int minor;
	struct cdev cdev;
	struct class *class;
	struct device *device;
	struct device_node *nd;
	struct timer_list timer; /* 定时器 */

	atomic_t keyvalue; /* 有效按键键值 */
	atomic_t releasekey; /* 标记是否完成一次完整的按键，包括按下和释放 */
	struct irq_keydesc irqkeydesc[KEY_NUM]; /* 按键描述数组 */
	unsigned char curkeynum;	/* 当前的按键号 */

	wait_queue_head_t r_wait;	/* 读等待队列头 */
};

/* irq device */
struct irq_dev irqdev;

/* @brief interrupt handler,to open timer, delay 10ms,timer 用于按键消抖 
 * @param irq interrupt number
 * @param dev_id struct of device
 * @return running result
 */
static irqreturn_t key0_handler(int irq, void *dev_id)
{
	struct irq_dev *dev = (struct irq_dev *)dev_id;
#if 1
	dev->curkeynum = 0;
	dev->timer.data = (volatile unsigned long)dev_id;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(20));	/* 20ms 定时 */
#endif

	/*tasklet_schedule(&dev->irqkeydesc[0].tasklet);*/
	/*schedule_work(&dev->work);*/

	return IRQ_RETVAL(IRQ_HANDLED);
}

/* tasklet */
static void key_tasklet(unsigned long data)
{
	struct irq_dev *dev = (struct irq_dev *)data;
	
	/*printk("key_tasklet\r\n");*/
	dev->timer.data = data;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(20));
}

#if 0
/* work */
static void key_work(struct work_struct *work)
{
	struct irq_dev *dev = container_of(work, struct irq_dev, work);

	dev->timer.data = (unsigned long)dev;
	mod_timer(&dev->timer, jiffies + msecs_to_jiffies(20));
}
#endif 

/* 定时器处理函数 */
static void timer_func(unsigned long arg)
{
	unsigned char value, num;
	struct irq_keydesc *keydesc;
	struct irq_dev *dev = (struct irq_dev *)arg;

	num = dev->curkeynum;
	keydesc = &dev->irqkeydesc[num];

	value = gpio_get_value(keydesc->gpio);
	/* press key */
	if (value == 0)
	{
		atomic_set(&dev->keyvalue, keydesc->value);
	}
	/* release key */
	else
	{
		atomic_set(&dev->keyvalue, 0x80 | keydesc->value);
		atomic_set(&dev->releasekey, 1); /* 标记松开按键，即完成一次完成的按键过程 */
	}

	/* 唤醒进程 */
	if (atomic_read(&dev->releasekey))
	{
		wake_up(&dev->r_wait);
	}
}

/* 初始化KEY */
static int key0_init(struct irq_dev *dev_id)
{
	int result = 0;
	unsigned char i = 0;
	struct irq_dev *dev = dev_id;

	/*dev->irqkeydesc[0].tasklet = key_tasklet;*/

	dev->nd = of_find_node_by_path("/key");
	if (dev->nd == NULL)
	{
		printk("key node not find!\r\n");
		result = -EINVAL;
		goto fail_nd;
	}

	/* 提取GPIO */
	for (i = 0; i < KEY_NUM; i++)
	{
		dev->irqkeydesc[i].gpio = of_get_named_gpio(dev->nd, "key-gpios", i);
		if (dev->irqkeydesc[i].gpio < 0)
		{
			printk("can't get key%d\r\n", i);
		}
	}

	/* 初始化key所使用的io ，并且设置成中断模式 */
	for (i = 0; i < KEY_NUM; i++)
	{
		memset(dev->irqkeydesc[i].name, 0, sizeof(dev->irqkeydesc[i].name));	/* 缓冲区清零 */
		sprintf(dev->irqkeydesc[i].name, "KEY%d", i);		/* 组合名字 */
		gpio_request(dev->irqkeydesc[i].gpio, dev->irqkeydesc[i].name);
		gpio_direction_input(dev->irqkeydesc[i].gpio);
		/*dev->irqkeydesc[i].irqnum = irq_of_parse_and_map(dev->nd, i);*/
#if 1
		dev->irqkeydesc[i].irqnum = gpio_to_irq(dev->irqkeydesc[i].gpio);
#endif
		printk("key%d:gpio= %d, irqnum = %d\r\n", i, dev->irqkeydesc[i].gpio, dev->irqkeydesc[i].irqnum);
	}

	/* 申请中断 */
	dev->irqkeydesc[0].handler = key0_handler;
	dev->irqkeydesc[0].value = KEY0VALUE;

	for (i = 0; i < KEY_NUM; i++)
	{
		result = request_irq(dev->irqkeydesc[i].irqnum, dev->irqkeydesc[i].handler, IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING, dev->irqkeydesc[i].name, dev);
		if (result < 0)
		{
			printk("irq %d request failed!\r\n", dev->irqkeydesc[i].irqnum);
			goto fail_irq;
		}

		/*tasklet_init(&dev->irqkeydesc[i].tasklet, key_tasklet, (unsigned long)dev);*/
	}

	/*INIT_WORK(&dev->work, key_work);*/

	/* create timer */
	init_timer(&dev->timer);
	dev->timer.function = timer_func;

	return 0;

fail_irq:
	for (i = 0; i < KEY_NUM; i++)
	{
		gpio_free(dev->irqkeydesc[i].gpio);
	}
fail_nd:
	return result;
}

static int irq_open(struct inode *inode, struct file *file)
{
	file->private_data = &irqdev;

	return 0;
}

static ssize_t irq_read(struct file *file, char __user *buf, size_t cnt, loff_t *offt)
{
	int ret = 0;
	unsigned char keyvalue = 0, releasekey = 0;
	struct irq_dev *dev = (struct irq_dev *)file->private_data;

#if 0
	/* 等待事件 */
	wait_event_interruptible(dev->r_wait, atomic_read(&dev->releasekey));	/* 等待按键有效 */
#endif

#if 0
	DECLARE_WAITQUEUE(wait, current);	/* 定义一个等待队列项 */
	if (atomic_read(&dev->releasekey) == 0)	/* 按键没有按下 */
	{
		add_wait_queue(&dev->r_wait, &wait);	/* 将队列项添加到等待队列头 */
		__set_current_state(TASK_INTERRUPTIBLE);	/* 当前进程设置为可被打断的状态 */
		schedule();	/* 切换 */

		/* 唤醒以后从这里运行 */
		if (signal_pending(current))	
		{
			ret = -ERESTARTSYS;
			goto data_error;
		}

		__set_current_state(TASK_RUNNING);	/* 将当前任务设置为运行状态 */
		remove_wait_queue(&dev->r_wait, &wait);	/* 将对应的队列项从等待队列头删除 */
	}
#endif 

	DECLARE_WAITQUEUE(wait, current);	/* 定义一个等待队列项 */
	add_wait_queue(&dev->r_wait, &wait);	/* 将队列项添加到等待队列头 */
	__set_current_state(TASK_INTERRUPTIBLE);	/* 当前进程设置为可被打断的状态 */
	schedule();	/* 切换 */

	/* 唤醒以后从这里运行 */
	if (signal_pending(current))	
	{
		ret = -ERESTARTSYS;
		goto data_error;
	}

	keyvalue = atomic_read(&dev->keyvalue);
	releasekey = atomic_read(&dev->releasekey);

	/* press key 有效按键 */
	if (releasekey)
	{
		if (keyvalue & 0x80)
		{
			keyvalue &= ~0x80;
			ret = __copy_to_user(buf, &keyvalue, sizeof(keyvalue));
		}
		else
		{
			goto data_error;
		}
		atomic_set(&dev->releasekey, 0);	/* 按下标志清零 */
	}
	else
	{
		goto data_error;
	}

	/*return 0;*/

data_error:
	__set_current_state(TASK_RUNNING);	/* 将当前任务设置为运行状态 */
	remove_wait_queue(&dev->r_wait, &wait);	/* 将对应的队列项从等待队列头删除 */

	return ret;
}

/* 设备操作函数 */
static struct file_operations irq_fops = 
{
	.owner = THIS_MODULE,
	.open = irq_open,
	.read = irq_read,
};

/* 驱动入口函数 */
static int __init imx6uirq_init(void)
{
	int result;

	irqdev.major = 0;
	if (irqdev.major)
	{
		irqdev.devid = MKDEV(irqdev.major, 0);
		result = register_chrdev_region(irqdev.devid, IRQ_CNT, IRQ_NAME);
	}
	else
	{
		result = alloc_chrdev_region(&irqdev.devid, 0, IRQ_CNT, IRQ_NAME);
		irqdev.major = MAJOR(irqdev.devid);
		irqdev.minor = MINOR(irqdev.minor);
	}
	if (result < 0)
	{
		goto fail_devid;
	}

	printk("irqdev major = %d, minor = %d \r\n", irqdev.major, irqdev.minor);

	irqdev.cdev.owner = THIS_MODULE;
	cdev_init(&irqdev.cdev, &irq_fops);

	result = cdev_add(&irqdev.cdev, irqdev.devid, IRQ_CNT);
	if (result)
	{
		goto fail_cdevadd;
	}

	irqdev.class = class_create(THIS_MODULE, IRQ_NAME);
	if (IS_ERR(irqdev.class))
	{
		result = PTR_ERR(irqdev.class);
		goto fail_class;
	}

	irqdev.device = device_create(irqdev.class, NULL, irqdev.devid, NULL, IRQ_NAME);
	if (IS_ERR(irqdev.device))
	{
		result = PTR_ERR(irqdev.device);
		goto fail_device;
	}

	/* 初始化按键 */
	atomic_set(&irqdev.keyvalue, INVAKEY);
	atomic_set(&irqdev.releasekey, 0);
	key0_init(&irqdev);

	/* 初始化等待队列头 */
	init_waitqueue_head(&irqdev.r_wait);

	return 0;

fail_device:
	class_destroy(irqdev.class);
fail_class:
	cdev_del(&irqdev.cdev);
fail_cdevadd:
	unregister_chrdev_region(irqdev.devid, IRQ_CNT);
fail_devid:
	return result;
}

/* 驱动出口函数 */
static void __exit imx6uirq_exit(void)
{
	unsigned int i = 0;

	del_timer_sync(&irqdev.timer);

	for (i = 0; i < KEY_NUM; i++)
	{
		free_irq(irqdev.irqkeydesc[i].irqnum, &irqdev);
	}
	for (i = 0; i < KEY_NUM; i++)
	{
		gpio_free(irqdev.irqkeydesc[i].gpio);
	}

	cdev_del(&irqdev.cdev);
	unregister_chrdev_region(irqdev.devid, IRQ_CNT);

	device_destroy(irqdev.class, irqdev.devid);
	class_destroy(irqdev.class);
}

module_init(imx6uirq_init);
module_exit(imx6uirq_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MZG");

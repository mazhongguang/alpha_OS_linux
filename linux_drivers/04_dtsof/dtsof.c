#include "linux/blk_types.h"
#include "linux/err.h"
#include "linux/gfp.h"
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
#include <linux/slab.h>

/* module entry */
static int __init dtsof_init(void)
{
	int result = 0;
	struct device_node *bl_nd = NULL; /* node */
	struct property *comppro = NULL;
	const char *str;
	u32 def_value = 0;
	u32 elemsize = 0;
	u32 *brival;
	uint8_t i;

	/* 1,找到backlight节点 , path = /backlight */
	bl_nd = of_find_node_by_path("/backlight");
	if (bl_nd == NULL)
	{
		result = -EINVAL;
		goto fail_findnd;
	}

	/* 2,获取属性 */
	comppro = of_find_property(bl_nd, "compatible", NULL);
	if (comppro == NULL)
	{
		result = -EINVAL;
		goto fail_finpro;
	}
	else
	{
		printk("compatible = %s\r\n", (char *)comppro->value);
	}

	result = of_property_read_string(bl_nd, "status", &str);
	if (result < 0)
	{
		goto fail_rs;
	}
	else
	{
		printk("status = %s \r\n", str);
	}

	/* 3,获取数字属性值 */
	result = of_property_read_u32(bl_nd, "default-brightness-level", &def_value);
	if (result < 0)
	{
		goto fail_read32;
	}
	else
	{
		printk("default-brightness-level = %d\r\n", def_value);
	}

	/* 4,获取数组类型的属性 */
	elemsize = of_property_count_elems_of_size(bl_nd, "brightness-levels", sizeof(u32));
	if (elemsize < 0)
	{
		result = -EINVAL;
		goto fail_readele;
	}
	else
	{
		printk("brightness-levels elems size = %d\r\n", elemsize);
	};

	/* 5,申请内存 */
	brival = kmalloc(elemsize * sizeof(u32), GFP_KERNEL);
	if (!brival)
	{
		result = -EINVAL;
		goto fail_mem;
	}

	/* 6,获取数组 */
	result = of_property_read_u32_array(bl_nd, "brightness-levels", brival, elemsize);
	if (result < 0)
	{
		goto fail_read32array;
	}
	else
	{
		for (i = 0; i < elemsize; i++)
		{
			printk("brightness-levels[%d] = %d\r\n", i, *(brival + i));
		}
	}

	return 0;

fail_read32array:
	kfree(brival); /* 释放内存 */
fail_mem:
fail_readele:
fail_read32:
fail_rs:
fail_finpro:
fail_findnd:
	return result;
}

/* module exit */
static void __exit dtsof_exit(void)
{
}

/* module entry and exit */
module_init(dtsof_init);
module_exit(dtsof_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MZG");

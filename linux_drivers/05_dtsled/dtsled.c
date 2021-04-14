#include "linux/blk_types.h"
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
#include <linux/slab.h>

/* module entry */

/* module entry and exit */
module_init(dtsled_init);
module_exit(dtsled_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("MZG");

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/string.h>
#include <linux/irq.h>
#include <asm/mach/map.h>
#include <asm/uaccess.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/fcntl.h>
#include <linux/ide.h>
#include <linux/platform_device.h>

#define PLATFORM_NAME  "platled"
#define PLATFORM_COUNT 1



/* 地址映射后的虚拟地址指针 */
static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

#define LEDOFF  0       /* 关闭 */
#define LEDON   1       /* 打开 */



/* LED设备结构体 */
struct newchrled_dev{
    struct cdev cdev;       /* 字符设备 */
    dev_t   devid;          /* 设备号 */
    struct class *class;    /* 类 */
    struct device *device;  /* 设备 */
    int major;              /* 主设备号 */
    int minor;              /* 次设备号 */

};

struct newchrled_dev newchrled; /* led设备 */

/* LED灯打开/关闭 */
static void led_switch(u8 sta)
{
    u32 val = 0;

    if(sta == LEDON) {
        val = readl(GPIO1_DR);
        val &= ~(1 << 3);            /* bit3清零,打开LED灯 */
        writel(val, GPIO1_DR); 
    } else if(sta == LEDOFF) {
        val = readl(GPIO1_DR);
        val |= (1 << 3);            /* bit3清零,打开LED灯 */
        writel(val, GPIO1_DR);
    }
}

static int newchrled_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &newchrled;
    return 0;
}

static int newchrled_release(struct inode *inode, struct file *filp)
{
    struct newchrled_dev *dev = (struct newchrled_dev*)filp->private_data;
    
    return 0;
}

static ssize_t newchrled_write(struct file *filp, const char __user *buf,
			 size_t count, loff_t *ppos)
{
    int retvalue;
    unsigned char databuf[1];

    retvalue = copy_from_user(databuf, buf, count);
    if(retvalue < 0) {
        printk("kernel write failed!\r\n");
        return -EFAULT;
    }

    /* 判断是开灯还是关灯 */
    led_switch(databuf[0]);

    return 0;
}

static const struct file_operations newchrled_fops = {
    .owner = THIS_MODULE,
    .write	= newchrled_write,
	.open	= newchrled_open,
	.release= newchrled_release,
};

static int led_probe(struct platform_device *dev)
{
  int i = 0;
  struct resource *ledsource[5];
  int ret = 0;
  unsigned int val = 0;

  //printk("led driver proe\r\n");
  /* 初始化LED，字符设备驱动 */
  /* 1,从设备中获取资源 */
  for(i=0; i < 5; i++) {
    ledsource[i] = platform_get_resource(dev, IORESOURCE_MEM, i);
    if (ledsource[i] == NULL)
		  return -EINVAL;
  }

    /* 内存映射 */
    /* 1,初始化LED灯,地址映射 */
    IMX6U_CCM_CCGR1 = ioremap(ledsource[0]->start, resource_size(ledsource[0]));
    SW_MUX_GPIO1_IO03 = ioremap(ledsource[1]->start, resource_size(ledsource[1]));
    SW_PAD_GPIO1_IO03 = ioremap(ledsource[2]->start, resource_size(ledsource[2]));
    GPIO1_DR = ioremap(ledsource[3]->start, resource_size(ledsource[3]));
    GPIO1_GDIR = ioremap(ledsource[4]->start, resource_size(ledsource[4]));

    /* 2,初始化 */
    val = readl(IMX6U_CCM_CCGR1);
    val &= ~(3 << 26);  /* 先清除以前的配置bit26,27 */
    val |= 3 << 26;     /* bit26,27置1 */
    writel(val, IMX6U_CCM_CCGR1);
 
    writel(0x5, SW_MUX_GPIO1_IO03);     /* 设置复用 */
    writel(0X10B0, SW_PAD_GPIO1_IO03);  /* 设置电气属性 */

    val = readl(GPIO1_GDIR);
    val |= 1 << 3;              /* bit3置1,设置为输出 */
    writel(val, GPIO1_GDIR);

    val = readl(GPIO1_DR);
    val |= (1 << 3);            /* bit3置1,关闭LED灯 */
    writel(val, GPIO1_DR);

    newchrled.major = 0;    /* 设置为0,表示由系统申请设备号 */

    /* 2，注册字符设备 */
    if(newchrled.major){    /* 给定主设备号 */
        newchrled.devid = MKDEV(newchrled.major, 0);
        ret = register_chrdev_region(newchrled.devid, PLATFORM_COUNT, PLATFORM_NAME);
    } else {                /* 没有给定主设备号 */
        ret = alloc_chrdev_region(&newchrled.devid, 0, PLATFORM_COUNT, PLATFORM_NAME);
        newchrled.major = MAJOR(newchrled.devid);
        newchrled.minor = MINOR(newchrled.devid);
    }
    if(ret < 0) {
        printk("newchrled chrdev_region err!\r\n");
        goto fail_devid;
    }
    printk("newchrled major=%d, minor=%d\r\n", newchrled.major, newchrled.minor);

    /* 3,注册字符设备 */
    newchrled.cdev.owner = THIS_MODULE;
    cdev_init(&newchrled.cdev, &newchrled_fops);
    ret = cdev_add(&newchrled.cdev, newchrled.devid, PLATFORM_COUNT);
    if(ret < 0) {
        goto fail_cdev;
    }

    /* 4,自动创建设备节点 */
    newchrled.class = class_create(THIS_MODULE, PLATFORM_NAME);
	if (IS_ERR(newchrled.class)) {
        ret = PTR_ERR(newchrled.class);
		goto fail_class;
    }

    newchrled.device = device_create(newchrled.class, NULL,
			     newchrled.devid, NULL, PLATFORM_NAME);
	if (IS_ERR(newchrled.device)) {
        ret = PTR_ERR(newchrled.device);
        goto fail_device;
    }
		
    return 0;

fail_device:
    class_destroy(newchrled.class);
fail_class:
    cdev_del(&newchrled.cdev);
fail_cdev:
    unregister_chrdev_region(newchrled.devid, PLATFORM_COUNT);
fail_devid:
	return ret; 
}

static int led_remove(struct platform_device *dev)
{

  unsigned int val = 0;

  printk("led driver remove\r\n");
  val = readl(GPIO1_DR);
  val |= (1 << 3);            /* bit3清零,打开LED灯 */
  writel(val, GPIO1_DR);

  /* 1,取消地址映射 */
  iounmap(IMX6U_CCM_CCGR1);
  iounmap(SW_MUX_GPIO1_IO03);
  iounmap(SW_PAD_GPIO1_IO03);
  iounmap(GPIO1_DR);
  iounmap(GPIO1_GDIR);

  /* 1,删除字符设备 */
  cdev_del(&newchrled.cdev);

  /* 2,注销设备号 */
  unregister_chrdev_region(newchrled.devid, PLATFORM_COUNT);

  /* 3,摧毁设备 */
  device_destroy(newchrled.class, newchrled.devid);
  
  /* 4,摧毁类 */
  class_destroy(newchrled.class);
  return 0;
}

/*
 platform驱动结构体
 */
static struct platform_driver led_driver = {
  .driver = {
    .name = "imx6ull-led",   /* 驱动名字，用于和设备匹配 */
  },
  .probe = led_probe,
  .remove = led_remove,
};

/*
 驱动加载
 */
static int __init leddriver_init(void)
{
    /* 注册platform驱动 */
    return platform_driver_register(&led_driver);
}

/* 
 驱动卸载
 */
static void __exit leddriver_exit(void)
{
  platform_driver_unregister(&led_driver);
}

module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zuozhongkai");
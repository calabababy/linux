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

/* 寄存器物理地址 */
#define CCM_CCGR1_BASE              (0X020C406C)
#define SW_MUX_GPIO1_IO03_BASE      (0X020E0068)
#define SW_PAD_GPIO1_IO03_BASE      (0X020E02F4)
#define GPIO1_DR_BASE               (0X0209C000)
#define GPIO1_GDIR_BASE             (0X0209C004)

#define REGISTER_LENGTH             4



void leddevice_release(struct device *dev)
{
    printk("leddevice release\r\n");
}

static struct resource led_resources[] = {
    [0] = {
		.start	= CCM_CCGR1_BASE,
		.end	= CCM_CCGR1_BASE + REGISTER_LENGTH - 1,
		.flags	= IORESOURCE_MEM, 
    },
    [1] = {
		.start	= SW_MUX_GPIO1_IO03_BASE,
		.end	= SW_MUX_GPIO1_IO03_BASE + REGISTER_LENGTH - 1,
		.flags	= IORESOURCE_MEM,
    },
    [2] = {
		.start	= SW_PAD_GPIO1_IO03_BASE,
		.end	= SW_PAD_GPIO1_IO03_BASE + REGISTER_LENGTH - 1,
		.flags	= IORESOURCE_MEM,
    },
    [3] = {
		.start	= GPIO1_DR_BASE,
		.end	= GPIO1_DR_BASE + REGISTER_LENGTH - 1,
		.flags	= IORESOURCE_MEM,
    },
    [4] = {
		.start	= GPIO1_GDIR_BASE,
		.end	= GPIO1_GDIR_BASE + REGISTER_LENGTH - 1,
		.flags	= IORESOURCE_MEM,
    },
}; 


static struct platform_device leddevice = {
    .name = "imx6ull-led",
    .id = -1,
    .dev = {
        .release  = leddevice_release,
    },
    .num_resources = ARRAY_SIZE(led_resources),
    .resource = led_resources,
};

/*
 设备加载
 */
static int __init leddevice_init(void)
{
    /* 注册platform设备 */
    return platform_device_register(&leddevice);
}

/* 
 设备卸载
 */
static void __exit leddevice_exit(void)
{
    platform_device_unregister(&leddevice);
}

module_init(leddevice_init);
module_exit(leddevice_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zuozhongkai");
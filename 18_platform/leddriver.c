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

#define PLATFORM_NAME "platformled"
#define PLATFORM_COUNT 1

static void __iomem *IMX6U_CCM_CCGR1;
static void __iomem *SW_MUX_GPIO1_IO03;
static void __iomem *SW_PAD_GPIO1_IO03;
static void __iomem *GPIO1_DR;
static void __iomem *GPIO1_GDIR;

#define LEDOFF 0
#define LEDON  1

/*led设备结构体*/
struct newchrled_dev{
	dev_t devid;
	int major;	//主设备号
	int minor;  //次设备号
	struct cdev cdev;
	struct class *class;
	struct device *device;
};

struct newchrled_dev newchrled;

static void led_switch(u8 sta)
{
		u32 val = 0;
	if(sta == LEDON){
			val = readl(GPIO1_DR);
			val &= ~(1 << 3);  //bit3清0 打开led
			writel(val,GPIO1_DR);
	}
	else if(sta == LEDOFF){
			val = readl(GPIO1_DR);
			val |= (1 << 3);  //bit3清0 关闭led
				writel(val,GPIO1_DR);
	}
 }


static int newchrled_open(struct inode *inode, struct file *filp){
	filp->private_data = &newchrled;
	return 0;
}

static int newchrled_release(struct inode *inode, struct file *filp){
//	struct newchrled_dev *dev = (struct newchrled_dev*)filp->private_data;

	return 0;
}

static ssize_t newchrled_write(struct file *filp, const char __user *buf,
                          size_t count, loff_t *ppos)
{
	int retvalue;
	unsigned char databuf[1];

	retvalue = copy_from_user(databuf,buf,count);
	if(retvalue < 0){
				printk("kernel write failed!\r\n");
				return -EFAULT;
	}

	/*判断开灯还是关灯*/
	led_switch(databuf[0]);

	return 0;
}

static const struct file_operations newchrled_fops = {
	.owner = THIS_MODULE,
	.write  = newchrled_write,
	.open   = newchrled_open,
	.release= newchrled_release,
};

static int led_probe(struct platform_device *dev)
{
    int i = 0;
    struct resource *ledsource[5];
    int ret = 0;
	/*初始化led*/
	unsigned int val = 0;
   // printk("led driver probe\r\n");
   for(i = 0;i < 5;i++) {
        ledsource[i] = platform_get_resource(dev,IORESOURCE_MEM,i);
        if(ledsource[i] == NULL){
            return -EINVAL;
        }
   }

 //   ledsource[0]->end-ledsource[1]->start + 1;

    IMX6U_CCM_CCGR1 = ioremap(ledsource[0]->start,resource_size(ledsource[0]));
	SW_MUX_GPIO1_IO03 = ioremap(ledsource[1]->start,resource_size(ledsource[1]));
	SW_PAD_GPIO1_IO03 = ioremap(ledsource[2]->start,resource_size(ledsource[2]));
	GPIO1_DR = ioremap(ledsource[3]->start,resource_size(ledsource[3]));
	GPIO1_GDIR = ioremap(ledsource[4]->start,resource_size(ledsource[4]));

		/* 2.初始化*/
	val = readl(IMX6U_CCM_CCGR1);
	val &= ~(3 << 26); /*先清除以前的配置bit26,27*/
	val |= 3 << 26;  //bit 26.27置1
	writel(val,IMX6U_CCM_CCGR1);

	writel(0x5,SW_MUX_GPIO1_IO03);//设置复用
	writel(0x10B0,SW_PAD_GPIO1_IO03);//设置电气属性

	val = readl(GPIO1_GDIR);
	val |= 1 << 3;  //bit 3置1 设置为输出
	writel(val,GPIO1_GDIR);

	val = readl(GPIO1_DR);
	val &= ~(1 << 3);  //bit3清0 打开led
	writel(val,GPIO1_DR);

	if(newchrled.major){
		newchrled.devid = MKDEV(newchrled.major,0);
		ret = register_chrdev_region(newchrled.devid, PLATFORM_COUNT, PLATFORM_NAME);
	}
	else {
		ret = alloc_chrdev_region(&newchrled.devid, 0, PLATFORM_COUNT, PLATFORM_NAME);
		newchrled.major = MAJOR(newchrled.devid);
		newchrled.minor = MINOR(newchrled.devid);
	}
	if(ret < 0){
		printk("newchrled chrdev_region err!\r\n");
		return -1;
	}
	printk("newchrled major=%d,minor=%d\r\n",newchrled.major,newchrled.minor);
	/*注册字符设备*/

	newchrled.cdev.owner = THIS_MODULE;
	cdev_init(&newchrled.cdev, &newchrled_fops);
	ret = cdev_add(&newchrled.cdev, newchrled.devid, PLATFORM_COUNT);

	/*自动创建设备节点*/

	newchrled.class = class_create(THIS_MODULE, PLATFORM_NAME);
	if (IS_ERR(newchrled.class))
	return PTR_ERR(newchrled.class);

	newchrled.device = device_create(newchrled.class,NULL,newchrled.devid,NULL,PLATFORM_NAME);
	if (IS_ERR(newchrled.device))
		return PTR_ERR(newchrled.device);


	return 0;
}

static int led_remove(struct platform_device *dev)
{
    unsigned int val = 0;
    printk("led driver remove\r\n");
	val = readl(GPIO1_DR);
	val |= (1 << 3);  //bit3清0 关闭led
	writel(val,GPIO1_DR);
		/*取消地址映射*/
	iounmap(IMX6U_CCM_CCGR1);
	iounmap(SW_MUX_GPIO1_IO03);
	iounmap(SW_PAD_GPIO1_IO03);
	iounmap(GPIO1_DR);
	iounmap(GPIO1_GDIR);


	/*删除字符设备*/
	cdev_del(&newchrled.cdev);
	/*注销设备号*/
	unregister_chrdev_region(newchrled.devid, PLATFORM_COUNT);

	/*摧毁设备*/
	device_destroy(newchrled.class,newchrled.devid);

	/*摧毁类*/
	class_destroy(newchrled.class);
    return 0;
}

static struct platform_driver led_driver = {
    .driver = {
        .name = "imx6ull-led",
    },
    .probe = led_probe,
    .remove = led_remove,
};

//驱动加载
static int __init leddriver_init(void)
{
    return platform_driver_register(&led_driver);

}


static void __exit leddriver_exit(void)
{
   platform_driver_unregister(&led_driver);
}


module_init(leddriver_init);
module_exit(leddriver_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("guozhijian");

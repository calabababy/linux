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


#define GPIOLED_CNT 1
#define GPIOLED_NAME "dtsplatled"
#define LED_ON 1 
#define LED_OFF 0 

/*设备结构体*/
struct gpioled_dev{
    dev_t devid;
    int major;
    int minor;
    struct cdev cdev;
    struct class *class;
    struct device *device;
    struct device_node *nd;
    int led_gpio;
};

struct gpioled_dev gpioled;

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int led_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &gpioled; /* 设置私有数据 */
	return 0;
}

/*
 * @description		: 从设备读取数据 
 * @param - filp 	: 要打开的设备文件(文件描述符)
 * @param - buf 	: 返回给用户空间的数据缓冲区
 * @param - cnt 	: 要读取的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 读取的字节数，如果为负值，表示读取失败
 */
static ssize_t led_read(struct file *filp, char __user *buf, size_t cnt, loff_t *offt)
{
	return 0;
}

/*
 * @description		: 向设备写数据 
 * @param - filp 	: 设备文件，表示打开的文件描述符
 * @param - buf 	: 要写给设备写入的数据
 * @param - cnt 	: 要写入的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t led_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int ret;
    unsigned char databuf[1];
    struct gpioled_dev *dev = filp->private_data;

    ret = copy_from_user(databuf,buf,cnt);

    if(ret < 0){
        return -ENAVAIL;

    }

    if(databuf[0] == LED_ON)gpio_set_value(dev->led_gpio,0);
    else if(databuf[0] == LED_OFF)gpio_set_value(dev->led_gpio,1);

	return 0;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int led_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static const struct file_operations led_fops = {
    .owner		=	THIS_MODULE,
	.read		=	led_read,
	.write		=	led_write,
	.open		=	led_open,
	.release	=	led_release,
	
};

static int led_probe(struct platform_device *dev)
{
//	printk("led probe\r\n");

	int ret = 0;
    /*注册字符设备驱动*/
    gpioled.major = 0;
    if(gpioled.major){
        gpioled.devid = MKDEV(gpioled.major,0);
        register_chrdev_region(gpioled.devid,GPIOLED_CNT,GPIOLED_NAME);
    } else {
        alloc_chrdev_region(&gpioled.devid,0,GPIOLED_CNT,GPIOLED_NAME);
        gpioled.major = MAJOR(gpioled.devid);
        gpioled.minor = MINOR(gpioled.devid);
    }
    printk("gpioled major = %d, minor = %d\r\n",gpioled.major,gpioled.minor);

    /*初始化cdev*/
    gpioled.cdev.owner = THIS_MODULE;
    cdev_init(&gpioled.cdev,&led_fops);

    /*添加cdev*/
    cdev_add(&gpioled.cdev,gpioled.devid,GPIOLED_CNT);

    /*创建类*/
    gpioled.class = class_create(THIS_MODULE,GPIOLED_NAME);
    if(IS_ERR(gpioled.class)) {
        return PTR_ERR(gpioled.class);
    }

    /*创建设备*/
    gpioled.device = device_create(gpioled.class,NULL,gpioled.devid,NULL,GPIOLED_NAME);
    if(IS_ERR(gpioled.device)) {
        return PTR_ERR(gpioled.device);
    }

    /*获取设备节点*/
    gpioled.nd = of_find_node_by_path("/gpioled");
    if(gpioled.nd == NULL){
        ret = -EINVAL;
        printk("can't get led gpio\r\n");
        goto fail_findnode;
    }

    /*获取led所对应的GPIO*/
    gpioled.led_gpio = of_get_named_gpio(gpioled.nd, "led-gpios", 0);
    if(gpioled.led_gpio < 0){
        printk("can't find led gpio\r\n");
        ret = -EINVAL;
        goto fail_findnode;
    }
    printk("led gpio num = %d\r\n",gpioled.led_gpio);

    /*申请io*/
    ret = gpio_request(gpioled.led_gpio,"led-gpio");
    if(ret){
        pr_err("Failed to request the led gpio\r\n");
        ret = -EINVAL;
        goto fail_findnode;
    }

    /*使用io，设置为输出*/
    ret = gpio_direction_output(gpioled.led_gpio,1);
    if(ret){
        goto fail_setoutput;
    }

    /*设置输出为低电平*/
    gpio_set_value(gpioled.led_gpio,0);


    return 0;
fail_setoutput:
    gpio_free(gpioled.led_gpio);
fail_findnode:
    return ret;
}

static int led_remove(struct platform_device *dev)
{
//	printk("led remove\r\n");
	gpio_set_value(gpioled.led_gpio,1);
    /*注销字符设备驱动*/
    cdev_del(&gpioled.cdev);
    unregister_chrdev_region(gpioled.devid,GPIOLED_CNT);

    device_destroy(gpioled.class,gpioled.devid);
    class_destroy(gpioled.class);

    /*释放io*/
    gpio_free(gpioled.led_gpio);
	return 0;
}

struct of_device_id led_of_match[] = {
	{.compatible = "guozhijiang,ggled"},
	{/*sen*/},
};

struct platform_driver led_driver = {
	.driver = {
		.name = "imx6ull-led",
		.of_match_table = led_of_match,

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

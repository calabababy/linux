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

#define KEY_CNT 1
#define KEY_NAME "key"
#define KEY0VALUE 0xF0
#define INVAKEY   0x00

/*设备结构体*/
struct key_dev{
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

struct key_dev key;

/*
 * @description		: 打开设备
 * @param - inode 	: 传递给驱动的inode
 * @param - filp 	: 设备文件，file结构体有个叫做private_data的成员变量
 * 					  一般在open的时候将private_data指向设备结构体。
 * @return 			: 0 成功;其他 失败
 */
static int key_open(struct inode *inode, struct file *filp)
{
//	filp->private_data = &key; /* 设置私有数据 */

   
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
static ssize_t key_read(struct file *filp, char __user *buf, size_t cnt, loff_t *ppos)
{
//    struct key_dev *dev = filp->private_data;
    int ret = 0;
	return ret;
}

/*
 * @description		: 向设备写数据 
 * @param - filp 	: 设备文件，表示打开的文件描述符
 * @param - buf 	: 要写给设备写入的数据
 * @param - cnt 	: 要写入的数据长度
 * @param - offt 	: 相对于文件首地址的偏移
 * @return 			: 写入的字节数，如果为负值，表示写入失败
 */
static ssize_t key_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{
    int ret =  0;
	return ret;
}

/*
 * @description		: 关闭/释放设备
 * @param - filp 	: 要关闭的设备文件(文件描述符)
 * @return 			: 0 成功;其他 失败
 */
static int led_release(struct inode *inode, struct file *filp)
{
//    struct key_dev *dev = filp->private_data;

	return 0;
}

static const struct file_operations led_fops = {
    .owner		=	THIS_MODULE,
	.read		=	key_read,
	.write		=	key_write,
	.open		=	key_open,
	.release	=	led_release,
	
};

static int keyio_init(struct key_dev *dev) {
    int ret = 0;

    dev->nd = of_find_node_by_path("/key");
    if(dev->nd == NULL) {
        ret = -EINVAL;
        goto fail_nd;
    }

    dev->key_gpio = of_get_named_gpio(dev->nd, "key-gpios",0);
    if(dev->key_gpio < 0) {
        ret = -EINVAL;
        goto fail_gpio;
    }

    ret = gpio_request(dev->key_gpio, "key0");
    if(ret){
        ret = -EBUSY;
        printk("IO %d can't request!\r\n",dev->key_gpio);
        goto fail_request;
    }

    ret = gpio_direction_input(dev->key_gpio);
    if(ret < 0){
        ret = -EINVAL;
        goto fail_input;
    }
    return 0;

fail_input:
    gpio_free(dev->key_gpio);
fail_request:
fail_gpio:
fail_nd:
    return ret;
}

static int __init key_init(void)
{
    int ret = 0;

    /*注册字符设备驱动*/
    key.major = 0;
    if(key.major){
        key.devid = MKDEV(key.major,0);
        ret = register_chrdev_region(key.devid,KEY_CNT,KEY_NAME);
    } else {
        ret = alloc_chrdev_region(&key.devid,0,KEY_CNT,KEY_NAME);
        key.major = MAJOR(key.devid);
        key.minor = MINOR(key.devid);
    }

    if(ret < 0) {
        goto fail_devid;
    }

    printk("key major = %d, minor = %d\r\n",key.major,key.minor);

    /*初始化cdev*/
    key.cdev.owner = THIS_MODULE;
    cdev_init(&key.cdev,&led_fops);

    /*添加cdev*/
    ret = cdev_add(&key.cdev,key.devid,KEY_CNT);
    if(ret)
        goto fail_cdevadd;

    /*创建类*/
    key.class = class_create(THIS_MODULE,KEY_NAME);
    if(IS_ERR(key.class)) {
        ret = PTR_ERR(key.class);
        goto fail_class;
    }

    /*创建设备*/
    key.device = device_create(key.class,NULL,key.devid,NULL,KEY_NAME);
    if(IS_ERR(key.device)) {
        ret = PTR_ERR(key.device);
        goto fail_device;
    }



    if(ret < 0){
        goto fail_device;
    }

    return 0;

fail_device:
    class_destroy(key.class);
fail_class:
    cdev_del(&key.cdev);
fail_cdevadd:
    unregister_chrdev_region(key.devid,KEY_CNT);
fail_devid:
    return ret;
}

static void __exit key_exit(void)
{
    
    /*注销字符设备驱动*/
    cdev_del(&key.cdev);
    unregister_chrdev_region(key.devid,KEY_CNT);

    device_destroy(key.class,key.devid);
    class_destroy(key.class);

    gpio_free(key.key_gpio);

}





module_init(key_init);
module_exit(key_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("guozhijian");
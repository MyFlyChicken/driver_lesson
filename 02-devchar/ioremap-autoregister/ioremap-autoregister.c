
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/device.h>
#define MYDEV_NAME "xxx_sample_chardev"

#define GPIOE_MODR 0x50006000
#define GPIOE_ODR 0x50006014
#define RCC_MP_AHB4_EN 0x50000a28 

struct LED1_K_ADDR
{
    int* led1_modr;
    int* led1_odr;
    int* led1_rcc;
};

//描述一个字符设备：应该有哪些属性：
//1.设备名称，2.设备号：主设备好，次设备号，3.对设备进行操作的结构体struct file_operations
struct xxx_sample_chardev
{
    const char* dev_name;
    int major;
    int minor;
    struct file_operations fops;
    //在设备类型描述内核的映射地址：
    struct LED1_K_ADDR maping_addr;
    //添加设备类，设备属性：
    struct class* mydev_class;
    struct device* mydev;
};

char kernel_buf[128] = {0};
//1.在全局中定义的 xxx_sample_charde的对象
struct xxx_sample_chardev my_chrdev;

//在内核定义对应的函数接口：
//与文件read对应的函数指针
ssize_t xxx_sample_chardev_read (struct file * file, char __user *userbuf, size_t size, loff_t * offset)
{
    //回调函数中参数：
    //参数1：就是用户进程中使用open内核中创建的struct file的实例的地址。
    //参数2：usrbuf:就是用户进程中数据的地址。
    //参数3: size,用户进程中要拷贝的字节数。
    //参数4：当前的文件的偏移量。单元也是字节。

    //调用copy_to_user从用户进程中获取数据：
    int ret = 0;
    //最后在内核对用户传过来的长度进行过滤。避免操作非法内存。
    //对内核中的内存使用要十分慎重，很容易造成内核的崩溃。
    if(size >= sizeof(kernel_buf))
    {
        size = sizeof(userbuf);
    }
    ret = copy_to_user(userbuf, kernel_buf + *offset, size);
    if(ret)
    {
        printk("copy_to_user failed");
        return -EIO;
    }
    printk("内核中的xxx_sample_chardev_read执行了\n");
    return size;
}
//与write对应的函数指针
ssize_t xxx_sample_chardev_write (struct file *file, const char __user *usrbuf, size_t size, loff_t *offset)
{
    //回调函数中参数：
    //参数1：就是用户进程中使用open内核中创建的struct file的实例的地址。
    //参数2：usrbuf:就是用户进程中数据的地址。
    //参数3: size,用户进程中要拷贝的字节数。
    //参数4：当前文件的偏移量。单元也是字节。

    //调用copy_from_user从用户进程中获取数据：
    int ret = 0;
    if(size >= sizeof(kernel_buf))
    {
        size = sizeof(usrbuf);
    }
    memset(kernel_buf,0,sizeof(kernel_buf));
    
    ret = copy_from_user(kernel_buf + *offset, usrbuf, size);
    if(ret)
    {
        printk("copy_from_usre failed");
        return -EIO;
    }
    
    if(kernel_buf[0] == '1')
    {
        //输出高电平：
        printk("111111111111\n");
        *my_chrdev.maping_addr.led1_odr |= 0x1 << 10;
    }
    else{
        //输出低电平
        *my_chrdev.maping_addr.led1_odr &= ~(0x1 << 10);
    }

    printk("内核中的xxx_sample_chardev_write执行了kf[0] = %s\n",kernel_buf);
    return size;
}

//与open对应的函数指针
int xxx_sample_chardev_open (struct inode *inode, struct file *file)
{

    printk("内核中的xxx_sample_chardev_open执行了\n");
    //先使能（开启）AHB4总线上GPIOE端口的电源及时钟频率：
    my_chrdev.maping_addr.led1_rcc = ioremap(RCC_MP_AHB4_EN,4);
    if(my_chrdev.maping_addr.led1_rcc == NULL)
    {
        printk("RCC失败\n");
        return -EIO;
    }
    *my_chrdev.maping_addr.led1_rcc |= 0x1 << 4;
    //映射 + 初始化：
    my_chrdev.maping_addr.led1_modr = ioremap(GPIOE_MODR,4);
    if(my_chrdev.maping_addr.led1_modr == NULL)
    {
        printk("MODR失败\n");
        return -EIO;
    }
    //初始化输出模式：
    *my_chrdev.maping_addr.led1_modr &= ~(0x1 << 21);
    *my_chrdev.maping_addr.led1_modr |= 0x1 << 20;

    my_chrdev.maping_addr.led1_odr = ioremap(GPIOE_ODR,4);
    if(my_chrdev.maping_addr.led1_odr == NULL)
    {
        printk("ODR失败\n");
        return -EIO;
    }
    //输出低电平：初始状态为灭
    *my_chrdev.maping_addr.led1_odr &= 0x0;

    return 0;
}
//与close对应函数指针：
int xxx_sample_chardev_release (struct inode *inode, struct file *file)
{
    printk("内核中的xxx_sample_chardev_release执行了\n");
    *my_chrdev.maping_addr.led1_odr &= 0x0;
    return 0;
}

//入口函数：
int __init my_test_module_init(void)
{
    printk("A模块的入口函数执行了");
    //申请资源，初始化并配置资源。
    //2.初始化对象（对这个设备对象中的属性进行初始化）
    //2.1设备的名字：
    my_chrdev.dev_name = MYDEV_NAME;
    //2.2设备操作时的回调方法：
    my_chrdev.fops.open = xxx_sample_chardev_open;
    my_chrdev.fops.read = xxx_sample_chardev_read;
    my_chrdev.fops.write = xxx_sample_chardev_write;
    my_chrdev.fops.release = xxx_sample_chardev_release;
    //2.3 申请内核中的设备号：申请主设备号，并且关联对此设备的操作方法：
    my_chrdev.major = register_chrdev(0,MYDEV_NAME,&my_chrdev.fops);
    if(my_chrdev.major < 0)
    {
        printk("申请设备号失败，及关联操作方法失败\n");
        return my_chrdev.major;
    }
    printk("申请到的设备号=%d\n",my_chrdev.major);

    //申请设备类：
    my_chrdev.mydev_class = class_create(THIS_MODULE,"MYLED");
    if(IS_ERR(my_chrdev.mydev_class))
    {
        printk("class_create失败\n");
        return PTR_ERR(my_chrdev.mydev_class);
    }

    //申请设备对象：向上提交uevent事件：建立了设备节点与设备号之间的关系。
    my_chrdev.mydev = device_create(my_chrdev.mydev_class,NULL,MKDEV(my_chrdev.major,0),NULL,MYDEV_NAME);
    if(IS_ERR(my_chrdev.mydev) )
    {
        printk("device_create失败\n");
        return PTR_ERR(my_chrdev.mydev);
    }


    return 0;
}

//出口函数：
void __exit my_test_module_exit(void)
{
    printk("出口函数执行了\n");//把调试信息放在了系统日志缓冲区，使用dmesg来显示。
    //清理资源。
    //unregister_chrdev(my_chrdev.major,MYDEV_NAME);
    iounmap(my_chrdev.maping_addr.led1_modr);
    iounmap(my_chrdev.maping_addr.led1_odr);
    iounmap(my_chrdev.maping_addr.led1_rcc);
    //先销毁设备，再销毁设备类：
    device_destroy(my_chrdev.mydev_class,MKDEV(my_chrdev.major,0));
    class_destroy(my_chrdev.mydev_class);
}

//指定许可：
MODULE_LICENSE("GPL");
MODULE_AUTHOR("gaowanxi, email:gaonetcom@163.com");
//指定入口及出口函数：
module_init(my_test_module_init);
module_exit(my_test_module_exit);
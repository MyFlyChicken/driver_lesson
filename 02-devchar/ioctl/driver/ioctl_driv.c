#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>

#include "../ioctl_app/ioctl_app.h"

#define MYDEV_NAME "devchar"

/************* 1.描述一个字符设备：应该有哪些属性 ***********/

/* 定义IO寄存器操作结构体，用于实现ioremap,选择的IO是GPIO1_A4 */
/* 根据资料查询GPIO1_A4的使用的是默认功能，不再对复用功能进行选择。默认为PU */
#define SYS_GRF_BASE          (0xFDC60000) /* GPIO1_A4的上下拉控制寄存器/复用功能寄存器 在SYS_GRF区域内 */
#define GPIO1_A4_IOMUX_OFFSET (0x0004) /* 复用功能选择 */
#define GPIO1_A4_P_OFFSET     (0x0080) /* 上下拉选择 */
#define GPIO1_A4_IOMUX        (SYS_GRF_BASE + GPIO1_A4_IOMUX_OFFSET)
#define GPIO1_A4_P            (SYS_GRF_BASE + GPIO1_A4_P_OFFSET)

#define GPIO1_BASE           (0xFE740000)
#define GPIO1_A4_MODR_OFFSET (0x08)
#define GPIO1_A4_ODR_OFFSET  (0x00)
#define GPIOE_MODR           (GPIO1_BASE + GPIO1_A4_MODR_OFFSET) //模式选择 输入/输出
#define GPIOE_ODR            (GPIO1_BASE + GPIO1_A4_ODR_OFFSET)  //输出控制
#define RCC_MP_AHB4_EN       0x50000a28

struct LED1_K_ADDR
{
    int* led1_ddr;
    int* led1_dr;
    int* led1_rcc;
    int* led1_iomux;
    int* led1_iop;
};

struct xxx_sample_chardev
{
    struct cdev* c_dev;

    //在设备类型描述内核的映射地址：
    struct LED1_K_ADDR maping_addr;
    //添加设备类，设备属性：
    struct class*  mydev_class;
    struct device* mydev;
};
/* 在全局中定义的 xxx_sample_charde的对象 */
struct xxx_sample_chardev my_chrdev;

/************* 2.定义fops的接口函数 ***********/
char kernel_buf[128] = {0};
/**
 * @brief read接口函数参数
 * @param [in] file 就是用户进程中使用open内核中创建的struct file的实例的地址
 * @param [in] userbuf usrbuf:就是用户进程中数据的地址
 * @param [in] size size,用户进程中要拷贝的字节数
 * @param [in] offset 从内核空间拷贝时的偏移量。单元也是字节
 * @return ssize_t 结果
 * 
 * @details 
 */
ssize_t
xxx_sample_chardev_read(struct file* file, char __user* userbuf, size_t size, loff_t* offset)
{
    //调用copy_to_user从用户进程中获取数据：
    int ret = 0;
    //最后在内核对用户传过来的长度进行过滤。避免操作非法内存。
    //对内核中的内存使用要十分慎重，很容易造成内核的崩溃。
    if (size >= sizeof(kernel_buf)) {
        size = sizeof(userbuf);
    }
    ret = copy_to_user(userbuf, kernel_buf + *offset, size);
    if (ret) {
        printk("copy_to_user failed");
        return -EIO;
    }
    printk("内核中的xxx_sample_chardev_read执行了\n");
    return size;
}
/**
 * @brief write接口函数
 * @param [in] file 就是用户进程中使用open内核中创建的struct file的实例的地址
 * @param [in] usrbuf usrbuf:就是用户进程中数据的地址
 * @param [in] size 用户进程中要拷贝的字节数
 * @param [in] offset 文件读写偏移量。单元也是字节
 * @return ssize_t 执行结果
 * 
 * @details 
 */
ssize_t
xxx_sample_chardev_write(struct file* file, const char __user* usrbuf, size_t size, loff_t* offset)
{
    //调用copy_from_user从用户进程中获取数据：
    int ret = 0;
    if (size >= sizeof(kernel_buf)) {
        size = sizeof(usrbuf);
    }
    memset(kernel_buf, 0, sizeof(kernel_buf));

    ret = copy_from_user(kernel_buf + *offset, usrbuf, size);
    if (ret) {
        printk("copy_from_usre failed");
        return -EIO;
    }

    if (kernel_buf[0] == '1') {
        //输出高电平：
        printk("111111111111\n");
        *my_chrdev.maping_addr.led1_dr = 0x00100010;
    }
    else {
        //输出低电平
        *my_chrdev.maping_addr.led1_dr = 0x00100000;
    }

    printk("内核中的xxx_sample_chardev_write执行了kf[0] = %s\n", kernel_buf);
    return size;
}

//上层ioctl最终回调的驱动中的函数：
long xxx_sample_chardev_ioctl(struct file* file, unsigned int request_cmd, unsigned long arg)
{
    int             ret = 0;
    enum LED_STATUS stauts;

    ret = copy_from_user(&stauts, (void*)arg, sizeof(stauts));
    if (ret) {
        printk("copy_from_usre failed");
        return -EIO;
    }
    switch (request_cmd) {
    case LED_CTL_CMD:
        switch (stauts) {
        case LED1_ON: //开灯
            *my_chrdev.maping_addr.led1_dr = 0x00100010;
            break;
        case LED1_OFF: //关灯
            *my_chrdev.maping_addr.led1_dr = 0x00100000;
            break;
        default:
            break;
        }
        break;
    case LED_GTE_CMD:
        /* code */
        break;
    default:
        break;
    }

    return 0;
}

/**
 * @brief open接口函数
 * @param [in] inode 
 * @param [in] file 
 * @return int 
 * 
 * @details 
 */
int xxx_sample_chardev_open(struct inode* inode, struct file* file)
{
    printk("内核中的xxx_sample_chardev_open执行了\n");

    my_chrdev.maping_addr.led1_iomux = ioremap(GPIO1_A4_IOMUX, 4);
    if (my_chrdev.maping_addr.led1_iomux == NULL) {
        printk("MODR失败\n");
        return -EIO;
    }
    // 初始化GPIO1A4为IO模式：
    /* 如果想要写入寄存器，则需要对应的写使能位为1
        bit31:16为写使能位，
        bit2:0 模式选择
     */
    *my_chrdev.maping_addr.led1_iomux = 0x00070000;

    my_chrdev.maping_addr.led1_iop = ioremap(GPIO1_A4_P, 4);
    if (my_chrdev.maping_addr.led1_iop == NULL) {
        printk("MODR失败\n");
        return -EIO;
    }
    //初始化GPIO1A4为上拉模式：
    /* 如果想要写入寄存器，则需要对应的写使能位为1
        bit31:16为写使能位，
        bit9:8为上下拉模式选择 */
    *my_chrdev.maping_addr.led1_iop = 0x03000100;

    my_chrdev.maping_addr.led1_ddr = ioremap(GPIOE_MODR, 4);
    if (my_chrdev.maping_addr.led1_ddr == NULL) {
        printk("MODR失败\n");
        return -EIO;
    }
    //初始化输出模式：
    *my_chrdev.maping_addr.led1_ddr = 0x00100010;

    my_chrdev.maping_addr.led1_dr = ioremap(GPIOE_ODR, 4);
    if (my_chrdev.maping_addr.led1_dr == NULL) {
        printk("ODR失败\n");
        return -EIO;
    }
    //输出低电平
    *my_chrdev.maping_addr.led1_dr = 0x00100010;

    return 0;
}
/**
 * @brief release接口函数
 * @param [in] inode 
 * @param [in] file 
 * @return int 
 * 
 * @details 
 */
int xxx_sample_chardev_release(struct inode* inode, struct file* file)
{
    printk("内核中的xxx_sample_chardev_release执行了\n");
    *my_chrdev.maping_addr.led1_dr = 0x00100010;
    return 0;
}
/************* 3.驱动入口/出口函数定义 ***********/
int __init my_test_module_init(void)
{
    int ret = 0;

    static struct file_operations fops = {.open           = xxx_sample_chardev_open,
                                          .read           = xxx_sample_chardev_read,
                                          .write          = xxx_sample_chardev_write,
                                          .unlocked_ioctl = xxx_sample_chardev_ioctl,
                                          .release        = xxx_sample_chardev_release};

    printk("A模块的入口函数执行了");
    //1.申请cdev
    my_chrdev.c_dev = cdev_alloc();
    if (NULL == my_chrdev.c_dev) {
        printk("cdev_alloc err\n");
        return -ENOMEM;
    }
    //2.cdev初始化
    cdev_init(my_chrdev.c_dev, &fops);
    //3.申请设备号
    ret = alloc_chrdev_region(&my_chrdev.c_dev->dev, 0, 1, MYDEV_NAME);
    if (ret) {
        printk("alloc_chrdev_region err\n");
        return ret;
    }
    printk("申请到的主设备号 = %d\n", MAJOR(my_chrdev.c_dev->dev));
    //4.把cdev对象添加到内核设备链表中：
    ret = cdev_add(my_chrdev.c_dev, my_chrdev.c_dev->dev, 1);
    if (ret) {
        printk("cdev_add error\n");
        return ret;
    }

    //5.申请设备类：
    my_chrdev.mydev_class = class_create(THIS_MODULE, "MYLED");
    if (IS_ERR(my_chrdev.mydev_class)) {
        printk("class_create失败\n");
        return PTR_ERR(my_chrdev.mydev_class);
    }

    printk("dev is %s", MYDEV_NAME);

    //6.申请设备对象：向上提交uevent事件：建立了设备节点与设备号之间的关系。
    my_chrdev.mydev =
        device_create(my_chrdev.mydev_class, NULL, my_chrdev.c_dev->dev, NULL, MYDEV_NAME);
    if (IS_ERR(my_chrdev.mydev)) {
        printk("device_create失败\n");
        return PTR_ERR(my_chrdev.mydev);
    }

    return 0;
}

//出口函数：
void __exit my_test_module_exit(void)
{
    printk("出口函数执行了\n"); //把调试信息放在了系统日志缓冲区，使用dmesg来显示。
    //先销毁设备，再销毁设备类：
    device_destroy(my_chrdev.mydev_class, my_chrdev.c_dev->dev);
    class_destroy(my_chrdev.mydev_class);
    cdev_del(my_chrdev.c_dev); //从内核中移除cdev，但是资源依然被占用
    unregister_chrdev_region(my_chrdev.c_dev->dev, 1);
    kfree(my_chrdev.c_dev);

    iounmap(my_chrdev.maping_addr.led1_ddr);
    iounmap(my_chrdev.maping_addr.led1_dr);
    iounmap(my_chrdev.maping_addr.led1_iomux);
    iounmap(my_chrdev.maping_addr.led1_iop);
}

/************* 4.指定模块相关内容 ***********/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("gaowanxi, email:gaonetcom@163.com");
/************* 5.指定入口及出口函数 ***********/
module_init(my_test_module_init);
module_exit(my_test_module_exit);

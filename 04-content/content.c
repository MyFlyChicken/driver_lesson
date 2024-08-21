#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/device.h>

#define MYDEV_NAME "devchar"

#define CONTENT_SPIN_LOCK 0
#define CONTENT_SENP      1
#define CONTENT_MUTEX     2

#define CONTENT_DEFAULT CONTENT_SPIN_LOCK

#if (CONTENT_DEFAULT == CONTENT_SPIN_LOCK)

#elif (CONTENT_DEFAULT == CONTENT_SENP)

#elif (CONTENT_DEFAULT == CONTENT_MUTEX)

#else
#error "CONTENT_DEFAULT is unvalid"
#endif

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
    const char*            dev_name;
    int                    major;
    int                    minor;
    struct file_operations fops;
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
    //tspi 时钟默认开启 不再开启
    // my_chrdev.maping_addr.led1_rcc = ioremap(RCC_MP_AHB4_EN, 4);
    // if (my_chrdev.maping_addr.led1_rcc == NULL) {
    //     printk("RCC失败\n");
    //     return -EIO;
    // }
    // *my_chrdev.maping_addr.led1_rcc |= 0x1 << 4;
    //映射 + 初始化：
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
#if (CONTENT_DEFAULT == CONTENT_SPIN_LOCK)

#elif (CONTENT_DEFAULT == CONTENT_SENP)

#elif (CONTENT_DEFAULT == CONTENT_MUTEX)

#else
#error "CONTENT_DEFAULT is unvalid"
#endif
    return 0;
}
/************* 3.驱动入口/出口函数定义 ***********/
int __init my_test_module_init(void)
{
    printk("A模块的入口函数执行了");
    //申请资源，初始化并配置资源。
    //2.初始化对象（对这个设备对象中的属性进行初始化）
    //2.1设备的名字：
    my_chrdev.dev_name = MYDEV_NAME;
    //2.2设备操作时的回调方法：
    my_chrdev.fops.open    = xxx_sample_chardev_open;
    my_chrdev.fops.read    = xxx_sample_chardev_read;
    my_chrdev.fops.write   = xxx_sample_chardev_write;
    my_chrdev.fops.release = xxx_sample_chardev_release;
    //2.3 申请内核中的设备号：申请主设备号，并且关联对此设备的操作方法：
    my_chrdev.major = register_chrdev(0, MYDEV_NAME, &my_chrdev.fops);
    if (my_chrdev.major < 0) {
        printk("申请设备号失败，及关联操作方法失败\n");
        return my_chrdev.major;
    }
    printk("申请到的设备号=%d\n", my_chrdev.major);

    //申请设备类：
    my_chrdev.mydev_class = class_create(THIS_MODULE, "MYLED");
    if (IS_ERR(my_chrdev.mydev_class)) {
        printk("class_create失败\n");
        return PTR_ERR(my_chrdev.mydev_class);
    }

    //申请设备对象：向上提交uevent事件：建立了设备节点与设备号之间的关系。
    my_chrdev.mydev =
        device_create(my_chrdev.mydev_class, NULL, MKDEV(my_chrdev.major, 0), NULL, MYDEV_NAME);
    if (IS_ERR(my_chrdev.mydev)) {
        printk("device_create失败\n");
        return PTR_ERR(my_chrdev.mydev);
    }
    printk("dev is %s", MYDEV_NAME);
#if (CONTENT_DEFAULT == CONTENT_SPIN_LOCK)

#elif (CONTENT_DEFAULT == CONTENT_SENP)

#elif (CONTENT_DEFAULT == CONTENT_MUTEX)

#else
#error "CONTENT_DEFAULT is unvalid"
#endif
    return 0;
}

//出口函数：
void __exit my_test_module_exit(void)
{
    printk("出口函数执行了\n"); //把调试信息放在了系统日志缓冲区，使用dmesg来显示。
    //清理资源。
    iounmap(my_chrdev.maping_addr.led1_ddr);
    iounmap(my_chrdev.maping_addr.led1_dr);
    //iounmap(my_chrdev.maping_addr.led1_rcc);
    //先销毁设备，再销毁设备类：
    device_destroy(my_chrdev.mydev_class, MKDEV(my_chrdev.major, 0));
    class_destroy(my_chrdev.mydev_class);
    unregister_chrdev(my_chrdev.major, MYDEV_NAME);
}

/************* 4.指定模块相关内容 ***********/
MODULE_LICENSE("GPL");
MODULE_AUTHOR("gaowanxi, email:gaonetcom@163.com");
/************* 5.指定入口及出口函数 ***********/
module_init(my_test_module_init);
module_exit(my_test_module_exit);

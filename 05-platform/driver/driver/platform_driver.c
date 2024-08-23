#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mod_devicetable.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/semaphore.h>

struct LED1_K_ADDR
{
    int* led1_ddr;
    int* led1_dr;
    int* led1_rcc;
    int* led1_iomux;
    int* led1_iop;
};

struct my_sample_chrdev
{
    struct cdev* c_dev;
    //在设备类型描述内核的映射地址：
    struct LED1_K_ADDR chr_map;
    //添加设备类，设备属性：
    struct class* class;
    struct device* dev;
    //等待队列的属性：
    wait_queue_head_t wait_queue;
    u8                condition;
    // #if (CONTENT_DEFAULT == CONTENT_SPIN_LOCK)
    //     spinlock_t lock;
    //     u8         status;
    // #elif (CONTENT_DEFAULT == CONTENT_SEMP)
    //     //添加同步机制信号量：
    //     struct semaphore sem;
    // #elif (CONTENT_DEFAULT == CONTENT_MUTEX)
    //     struct mutex mtx;
    // #else
    // #error "CONTENT_DEFAULT is unvalid"
    // #endif
};

struct my_sample_chrdev my_chrdev = {0};

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

    if (file->f_flags & O_NONBLOCK) {
        if (my_chrdev.condition == 0) {
            return -EAGAIN;
        }
        else {
            if (size >= sizeof(kernel_buf)) {
                size = sizeof(userbuf);
            }
            ret = copy_to_user(userbuf, kernel_buf + *offset, size);
            if (ret) {
                printk("copy_to_user failed");
                my_chrdev.condition = 0;
                return -EIO;
            }
            my_chrdev.condition = 0;
            printk("内核中的xxx_sample_chardev_read执行了1\n");
            return size;
        }
    }

    wait_event_interruptible(my_chrdev.wait_queue, my_chrdev.condition);
    my_chrdev.condition = 0;
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
        *my_chrdev.chr_map.led1_dr = 0x00100010;
    }
    else {
        //输出低电平
        *my_chrdev.chr_map.led1_dr = 0x00100000;
    }

    printk("内核中的xxx_sample_chardev_write执行了kf[0] = %s\n", kernel_buf);

    my_chrdev.condition = 1;
    wake_up(&my_chrdev.wait_queue);

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
    int ret;

    (void)ret;
    printk("内核中的xxx_sample_chardev_open执行了\n");

    // #if (CONTENT_DEFAULT == CONTENT_SPIN_LOCK)
    //     (void)ret;
    //     spin_lock(&my_chrdev.lock);
    //     if (my_chrdev.status != 0) {
    //         spin_unlock(&my_chrdev.lock);
    //         return -EBUSY;
    //     }
    //     my_chrdev.status = 1;
    //     spin_unlock(&my_chrdev.lock);
    //     printk("当前获取自旋锁的进程为 = %d\n", current->pid);
    // #elif (CONTENT_DEFAULT == CONTENT_SEMP)
    //     ret = down_trylock(&my_chrdev.sem);
    //     if (ret) {
    //         return -EBUSY;
    //     }
    //     printk("当前获取信号量的进程为 = %d\n", current->pid);
    // #elif (CONTENT_DEFAULT == CONTENT_MUTEX)
    //     ret = mutex_trylock(&my_chrdev.mtx);
    //     if (ret == 0) {
    //         return -EBUSY;
    //     }
    //     printk("当前获取互斥量的进程为 = %d\n", current->pid);
    // #endif

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
    *my_chrdev.chr_map.led1_dr = 0x00100010;
    // #if (CONTENT_DEFAULT == CONTENT_SPIN_LOCK)
    //     my_chrdev.status = 0;
    //     spin_unlock(&my_chrdev.lock);
    // #elif (CONTENT_DEFAULT == CONTENT_SEMP)
    //     up(&my_chrdev.sem);
    // #elif (CONTENT_DEFAULT == CONTENT_MUTEX)
    //     mutex_unlock(&my_chrdev.mtx);
    // #else
    // #error "CONTENT_DEFAULT is unvalid"
    // #endif
    return 0;
}

struct file_operations fops = {
    .open    = xxx_sample_chardev_open,
    .read    = xxx_sample_chardev_read,
    .write   = xxx_sample_chardev_write,
    .release = xxx_sample_chardev_release,
};

int my_dev_driver_probe(struct platform_device* pdev)
{
    printk("哦恭喜，匹配上了\n");

    /* 申请并初始化 */
    my_chrdev.c_dev = cdev_alloc();
    cdev_init(my_chrdev.c_dev, &fops);

    /* 申请设备号并添加到设备管理链表中 */
    alloc_chrdev_region(&my_chrdev.c_dev->dev, 0, 1, pdev->name);
    cdev_add(my_chrdev.c_dev, my_chrdev.c_dev->dev, 1);

    /* 设备自动创建 */
    my_chrdev.class = class_create(THIS_MODULE, "mychr");
    my_chrdev.dev   = device_create(my_chrdev.class, NULL, my_chrdev.c_dev->dev, NULL, pdev->name);

    /* 提取资源文件，用于操作设备 */
    my_chrdev.chr_map.led1_iomux = ioremap(pdev->resource[0].start, 4);
    printk("iomux:%#llx\n", pdev->resource[0].start);
    *my_chrdev.chr_map.led1_iomux = 0x00070000;

    my_chrdev.chr_map.led1_iop = ioremap(pdev->resource[1].start, 4);
    printk("iop:%#llx\n", pdev->resource[1].start);
    *my_chrdev.chr_map.led1_iop = 0x03000100;

    my_chrdev.chr_map.led1_ddr = ioremap(pdev->resource[2].start, 4);
    printk("ioddr:%#llx\n", pdev->resource[2].start);
    *my_chrdev.chr_map.led1_ddr = 0x00100010;

    my_chrdev.chr_map.led1_dr = ioremap(pdev->resource[3].start, 4);
    printk("iodr:%#llx\n", pdev->resource[3].start);
    *my_chrdev.chr_map.led1_dr = 0x00100010;

    printk("irq:%d\n", platform_get_irq(pdev, 0));

    return 0;
}

int my_dev_driver_probe_remove(struct platform_device* pdev)
{
    printk("啊哦，要断开了\n");
    device_destroy(my_chrdev.class, my_chrdev.c_dev->dev);
    class_destroy(my_chrdev.class);

    // cdev_del(my_chrdev.c_dev); //从内核中移除cdev，但是资源依然被占用
    // unregister_chrdev_region(my_chrdev.c_dev->dev, 1);

    // iounmap(my_chrdev.maping_addr.led1_ddr);
    // iounmap(my_chrdev.maping_addr.led1_dr);
    // iounmap(my_chrdev.maping_addr.led1_iomux);
    // iounmap(my_chrdev.maping_addr.led1_iop);

    kfree(my_chrdev.c_dev);
    return 0;
}

/* id_table匹配 */
struct platform_device_id id_table_match[] = {
    [0] = {"yyf,device01", 1},
    [1] = {"yyf,device02", 2},
    [2] = {/*最后一个一定要给一个空元素，代表结束*/}
};

/* 名字匹配 */
struct platform_driver my_platform_driver = {
    .probe  = my_dev_driver_probe,
    .remove = my_dev_driver_probe,
    .driver =
        {
                 .name = "yyf,device01",
                 },
    .id_table = id_table_match,
};

//入口函数：
int __init my_test_module_init(void)
{
    int ret = 0;
    ret     = platform_driver_register(&my_platform_driver);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

//出口函数：
void __exit my_test_module_exit(void)
{
    platform_driver_unregister(&my_platform_driver);
}

//指定许可：
MODULE_LICENSE("GPL");
MODULE_AUTHOR("gaowanxi, email:gaonetcom@163.com");
//指定入口及出口函数：
module_init(my_test_module_init);
module_exit(my_test_module_exit);
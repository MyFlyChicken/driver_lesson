#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define MYDEV_NAME "xxx_sample_chardev"

/************* 1.描述一个字符设备：应该有哪些属性 ***********/
struct xxx_sample_chardev
{
    const char*            dev_name; /* 设备名称 */
    int                    major;    /* 设备号：主设备号 */
    int                    minor;    /* 设备号：次设备号 */
    struct file_operations fops;     /* 对设备进行操作的结构体struct file_operations */
};
//在全局中定义的 xxx_sample_chardev的对象
struct xxx_sample_chardev my_chrdev;

/************* 2.定义fops的接口函数 ***********/
/* 定义内核驱动缓存 */
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
    ret = copy_from_user(kernel_buf + *offset, usrbuf, size);
    if (ret) {
        printk("copy_from_usre failed");
        return -EIO;
    }
    printk("内核中的xxx_sample_chardev_write执行了\n");
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
    return 0;
}

/************* 3.驱动入口/出口函数定义 ***********/
/**
 * @brief 入口函数
 * @return int 执行结果
 * 
 * @details 
 */
int __init chr_virtual_module_init(void)
{
    printk("A模块的入口函数执行了");

    memset(kernel_buf, 0, sizeof(kernel_buf));

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
    return 0;
}

/**
 * @brief 出口函数
 * 
 * @details 
 */
void __exit chr_virtual_module_exit(void)
{
    printk("出口函数执行了\n"); //把调试信息放在了系统日志缓冲区，使用dmesg来显示。
    //清理资源。
    unregister_chrdev(my_chrdev.major, MYDEV_NAME);
}

/************* 4.指定模块相关内容 ***********/
//指定许可：
MODULE_LICENSE("GPL");
MODULE_AUTHOR("yyf");

/************* 5.指定入口及出口函数 ***********/
module_init(chr_virtual_module_init);
module_exit(chr_virtual_module_exit);

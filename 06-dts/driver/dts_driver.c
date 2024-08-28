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

#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/of_platform.h>
#include <linux/gpio.h>
#include <asm-generic/gpio.h>

struct dts_chrdev_t
{
    struct cdev* c_dev;

    //添加设备类，设备属性：
    struct class* class;
    struct device* dev;

    //等待队列的属性：
    wait_queue_head_t wait_queue;
    u8                condition;

    u32 chrdev1;
    u32 chrdev2;
    u32 chrdev3;
};

struct dts_chrdev_t dts_chrdev = {0};

char kernel_buf[128] = {0};

int dts_chrdev_open(struct inode* inode, struct file* file)
{
    printk("dts_chrdev_open执行了\n");
    return 0;
}

ssize_t dts_chrdev_read(struct file* file, char __user* userbuf, size_t size, loff_t* offset)
{
    int ret = 0;

    printk("dts_chrdev_read执行了\n");

    if (file->f_flags & O_NONBLOCK) {
        if (dts_chrdev.condition == 0) {
            return -EAGAIN;
        }
        else {
            if (size >= sizeof(kernel_buf)) {
                size = sizeof(userbuf);
            }
            ret = copy_to_user(userbuf, kernel_buf + *offset, size);
            if (ret) {
                printk("copy_to_user failed");
                dts_chrdev.condition = 0;
                return -EIO;
            }
            dts_chrdev.condition = 0;
            return size;
        }
    }

    wait_event_interruptible(dts_chrdev.wait_queue, dts_chrdev.condition);
    dts_chrdev.condition = 0;
    if (size >= sizeof(kernel_buf)) {
        size = sizeof(userbuf);
    }
    ret = copy_to_user(userbuf, kernel_buf + *offset, size);
    if (ret) {
        printk("copy_to_user failed");
        return -EIO;
    }

    return size;
}

ssize_t dts_chrdev_write(struct file* file, const char __user* usrbuf, size_t size, loff_t* offset)
{
    int ret = 0;

    printk("dts_chrdev_write执行了\n");

    if (size >= sizeof(kernel_buf)) {
        size = sizeof(usrbuf);
    }
    memset(kernel_buf, 0, sizeof(kernel_buf));

    //调用copy_from_user从用户进程中获取数据：
    ret = copy_from_user(kernel_buf + *offset, usrbuf, size);
    if (ret) {
        printk("copy_from_usre failed");
        return -EIO;
    }

    //TODO 编写设备树
    if (kernel_buf[0] == '1') {
        printk("--->>>1<<<---\n");
        gpio_set_value(dts_chrdev.chrdev1, 1);
    }
    else if (kernel_buf[0] == '2') {
        printk("--->>>2<<<---\n");
        gpio_set_value(dts_chrdev.chrdev2, 1);
    }
    else if (kernel_buf[0] == '3') {
        printk("--->>>3<<<---\n");
        gpio_set_value(dts_chrdev.chrdev3, 1);
    }
    else {
        printk("--->>>other<<<---\n");
        gpio_set_value(dts_chrdev.chrdev1, 0);
        gpio_set_value(dts_chrdev.chrdev2, 0);
        gpio_set_value(dts_chrdev.chrdev3, 0);
    }

    printk("内核中的xxx_sample_chardev_write执行了kf[0] = %s\n", kernel_buf);

    dts_chrdev.condition = 1;
    wake_up(&dts_chrdev.wait_queue);

    return size;
}

int dts_chrdev_release(struct inode* inode, struct file* file)
{
    printk("dts_chrdev_release执行了\n");
    return 0;
}

struct file_operations fops = {
    .open    = dts_chrdev_open,
    .read    = dts_chrdev_read,
    .write   = dts_chrdev_write,
    .release = dts_chrdev_release,
};

int chrdev_driver_probe(struct platform_device* pdev)
{
    int ret1, ret2, ret3;
    printk("恭喜，匹配上了, %s\n", pdev->name);

    /* 申请并初始化 */
    dts_chrdev.c_dev = cdev_alloc();
    cdev_init(dts_chrdev.c_dev, &fops);

    /* 申请设备号并添加到设备管理链表中 */
    alloc_chrdev_region(&dts_chrdev.c_dev->dev, 0, 1, pdev->name);
    cdev_add(dts_chrdev.c_dev, dts_chrdev.c_dev->dev, 1);

    /* 设备自动创建 */
    dts_chrdev.class = class_create(THIS_MODULE, "mychr");
    dts_chrdev.dev = device_create(dts_chrdev.class, NULL, dts_chrdev.c_dev->dev, NULL, pdev->name);

    /* 初始化等待队列头 */

    init_waitqueue_head(&dts_chrdev.wait_queue);
    dts_chrdev.condition = 0;

    /*TODO 获取设备树资源 */
    printk("111\n");
    /**
     * 参数：
     * np:pdev内的设备树节点指针
     * propname:属性名，必须为设备树内的修饰资源的属性名，如
     * chrdev_gpios = <&gpio3 RK_PB1 GPIO_ACTIVE_HIGH>,<&gpio3 RK_PB2 GPIO_ACTIVE_HIGH>,<&gpio0 RK_PB7 GPIO_ACTIVE_HIGH>，此处需要填写chrdev_gpios
     * index：下标，即gpio资源是一个列表时，相应的gpio资源的索引值。从0开始。
     * 成功返回 gpio编号，失败则为错误码。
     * 所以获取设备节点，也是前提哦。
     */
    dts_chrdev.chrdev1 = of_get_named_gpio(pdev->dev.of_node, "chrdev_gpios", 0);
    printk("222\n");
    if (dts_chrdev.chrdev1 < 0) {
        printk("dts_chrdev.chrdev1 error\n");
        goto error;
    }
    /**
     * 功能：申请当前进程使用GPIO号，防止进程竞态出现。
     * 参数：
     * gpio号 （设备树中获取）这里很关键。
     * label:用于标识该 GPIO 的标签。不使用，可以写NULL,主要作用是这个标签可以帮助在调试或日志中识别哪个模块或驱动程序请求了这个 GPIO。
     * 返回值，成功返回0，失败返回错误码。
     */
    ret1 = gpio_request(dts_chrdev.chrdev1, "chrdev1_gpios");
    printk("333\n");
    if (ret1 < 0) {
        printk("ret1 error\n");
        goto error;
    }
    gpio_direction_output(dts_chrdev.chrdev1, 0);
    printk("444\n");
    dts_chrdev.chrdev2 = of_get_named_gpio(pdev->dev.of_node, "chrdev_gpios", 1);
    printk("555\n");
    if (dts_chrdev.chrdev2 < 0) {
        printk("dts_chrdev.chrdev2 error\n");
        goto error;
    }
    ret2 = gpio_request(dts_chrdev.chrdev2, "chrdev2_gpios");
    printk("666\n");
    if (ret2 < 0) {
        printk("ret2 error\n");
        goto error;
    }
    gpio_direction_output(dts_chrdev.chrdev2, 0);
    dts_chrdev.chrdev3 = of_get_named_gpio(pdev->dev.of_node, "chrdev_gpios", 2);
    printk("777\n");
    if (dts_chrdev.chrdev3 < 0) {
        printk("dts_chrdev.chrdev3 error\n");
        goto error;
    }
    ret3 = gpio_request(dts_chrdev.chrdev3, "chrdev3_gpios");
    if (ret3 < 0) {
        printk("ret3 error\n");
        goto error;
    }
    gpio_direction_output(dts_chrdev.chrdev3, 0);

    return 0;
error:
    return -1;
}

int chrdev_driver_remove(struct platform_device* pdev)
{
    printk("啊哦，要断开了\n");

    //TODO 设备树
    gpio_free(dts_chrdev.chrdev1);
    gpio_free(dts_chrdev.chrdev2);
    gpio_free(dts_chrdev.chrdev3);

    device_destroy(dts_chrdev.class, dts_chrdev.c_dev->dev);
    class_destroy(dts_chrdev.class);

    cdev_del(dts_chrdev.c_dev); //从内核中移除cdev，但是资源依然被占用
    unregister_chrdev_region(dts_chrdev.c_dev->dev, 1);

    kfree(dts_chrdev.c_dev);
    return 0;
}

/* id_table方式匹配 */
struct platform_device_id id_table_match[] = {
    [0] = {"yyf,device01", 1},
    [1] = {"yyf,device02", 2},
    [2] = {/*最后一个一定要给一个空元素，代表结束*/}
};

struct of_device_id of_node_match_table[] = {[0] = {.compatible = "yyf,device01"},
                                             [1] = {.compatible = "yyf,device02"},
                                             [2] = {/*最后一个一定要给一个空元素，代表结束*/}};

/* 名字匹配 */
struct platform_driver chrdev_platform_driver = {
    .probe  = chrdev_driver_probe,
    .remove = chrdev_driver_remove,
    .driver =
        {
                 .name = "yyf,chrdev_driver",
                 /* 设备树方式匹配 */
            .of_match_table = of_node_match_table,
                 },
    /* id_table方式匹配 */
    .id_table = id_table_match,
};

//入口函数：
int __init chrdev_module_init(void)
{
    int ret = 0;
    ret     = platform_driver_register(&chrdev_platform_driver);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

//出口函数：
void __exit chrdev_module_exit(void)
{
    platform_driver_unregister(&chrdev_platform_driver);
}

//指定许可：
MODULE_LICENSE("GPL");
MODULE_AUTHOR("gaowanxi, email:gaonetcom@163.com");
//指定入口及出口函数：
module_init(chrdev_module_init);
module_exit(chrdev_module_exit);
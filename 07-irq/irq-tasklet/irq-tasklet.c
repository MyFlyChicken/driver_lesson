//TODO kernel 4.9.232 不支持tasklet_setup
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
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/delay.h>

struct dts_chrdev_t
{
    struct cdev* c_dev;

    //添加设备类，设备属性：
    struct class* class;
    struct device* dev;

    //等待队列的属性：
    wait_queue_head_t wait_queue;
    u8                condition;

    u32 led1;
    u32 led2;
    u32 led3;
};

struct dts_chrdev_t dts_led = {0};

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
        if (dts_led.condition == 0) {
            return -EAGAIN;
        }
        else {
            if (size >= sizeof(kernel_buf)) {
                size = sizeof(userbuf);
            }
            ret = copy_to_user(userbuf, kernel_buf + *offset, size);
            if (ret) {
                printk("copy_to_user failed");
                dts_led.condition = 0;
                return -EIO;
            }
            dts_led.condition = 0;
            return size;
        }
    }

    wait_event_interruptible(dts_led.wait_queue, dts_led.condition);
    dts_led.condition = 0;
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

    if (kernel_buf[0] == '1') {
        printk("--->>>1<<<---\n");
        gpio_set_value(dts_led.led1, 1);
    }
    else if (kernel_buf[0] == '2') {
        printk("--->>>2<<<---\n");
        gpio_set_value(dts_led.led2, 1);
    }
    else if (kernel_buf[0] == '3') {
        printk("--->>>3<<<---\n");
        gpio_set_value(dts_led.led3, 1);
    }
    else {
        printk("--->>>other<<<---\n");
        gpio_set_value(dts_led.led1, 0);
        gpio_set_value(dts_led.led2, 0);
        gpio_set_value(dts_led.led3, 0);
    }

    printk("内核中的xxx_sample_chardev_write执行了kf[0] = %s\n", kernel_buf);

    dts_led.condition = 1;
    wake_up(&dts_led.wait_queue);

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

struct keydev_t
{
    u32 key1_interrupts;
    u8  key1_gpios;
    u32 key2_interrupts;
    u8  key2_gpios;
    u32 key3_interrupts;
    u8  key3_gpios;
    //添加一个tasklet：
    struct tasklet_struct key_tasklet;
    //添加一下irq属性：
    volatile u32 irq;
};
struct keydev_t keydev = {0};

void tasklet_function(struct tasklet_struct* task)
{
    mdelay(100);
    if (keydev.irq == keydev.key1_interrupts) {
        if (gpio_get_value(keydev.key1_gpios) == 0) {
            //key1按下了：开1灯
            gpio_set_value(dts_led.led1, 1);
        }
        else {
            //key1抬起了：关1灯：
            gpio_set_value(dts_led.led1, 0);
        }
    }
    if (keydev.irq == keydev.key2_interrupts) {
        if (gpio_get_value(keydev.key2_gpios) == 0) {
            //key1按下了：开1灯
            gpio_set_value(dts_led.led2, 1);
        }
        else {
            //key1抬起了：关1灯：
            gpio_set_value(dts_led.led2, 0);
        }
    }
    if (keydev.irq == keydev.key3_interrupts) {
        if (gpio_get_value(keydev.key3_gpios) == 0) {
            //key1按下了：开1灯
            gpio_set_value(dts_led.led3, 1);
        }
        else {
            //key1抬起了：关1灯：
            gpio_set_value(dts_led.led3, 0);
        }
    }
}

irqreturn_t key_ISR(int irq, void* dev)
{
    keydev.irq = irq;

    //启动定时器：
    tasklet_schedule(&keydev.key_tasklet);
    return IRQ_HANDLED;
}

int chrdev_driver_probe(struct platform_device* pdev)
{
    struct device_node* key_node;
    int                 ret1, ret2, ret3;
    printk("恭喜，匹配上了, %s\n", pdev->name);

    /* 申请并初始化 */
    dts_led.c_dev = cdev_alloc();
    cdev_init(dts_led.c_dev, &fops);

    /* 申请设备号并添加到设备管理链表中 */
    alloc_chrdev_region(&dts_led.c_dev->dev, 0, 1, pdev->name);
    cdev_add(dts_led.c_dev, dts_led.c_dev->dev, 1);

    /* 设备自动创建 */
    dts_led.class = class_create(THIS_MODULE, "mychr");
    dts_led.dev   = device_create(dts_led.class, NULL, dts_led.c_dev->dev, NULL, pdev->name);

    /* 初始化等待队列头 */
    init_waitqueue_head(&dts_led.wait_queue);
    dts_led.condition = 0;

    dts_led.led1 = of_get_named_gpio(pdev->dev.of_node, "chrdev_gpios", 0);
    if (dts_led.led1 < 0) {
        printk("dts_led.led1 error\n");
        goto error;
    }
    ret1 = gpio_request(dts_led.led1, "chrdev1_gpios");
    if (ret1 < 0) {
        printk("ret1 error\n");
        goto error;
    }
    gpio_direction_output(dts_led.led1, 0);

    dts_led.led2 = of_get_named_gpio(pdev->dev.of_node, "chrdev_gpios", 1);
    if (dts_led.led2 < 0) {
        printk("dts_led.led2 error\n");
        goto error;
    }
    ret2 = gpio_request(dts_led.led2, "chrdev2_gpios");
    if (ret2 < 0) {
        printk("ret2 error\n");
        goto error;
    }
    gpio_direction_output(dts_led.led2, 0);

    dts_led.led3 = of_get_named_gpio(pdev->dev.of_node, "chrdev_gpios", 2);
    if (dts_led.led3 < 0) {
        printk("dts_led.led3 error\n");
        goto error;
    }
    ret3 = gpio_request(dts_led.led3, "chrdev3_gpios");
    if (ret3 < 0) {
        printk("ret3 error\n");
        goto error;
    }
    gpio_direction_output(dts_led.led3, 0);

    //获取key设备树象节点的对象指针：
    key_node = of_find_node_by_path("/keydev");
    printk("key_node = %llx\n", (long long int)key_node);

    //获取Key对象的linux中断号：
    keydev.key1_interrupts = of_irq_get(key_node, 0);
    keydev.key1_gpios      = of_get_named_gpio(key_node, "keydev_gpios", 0);
    printk("keydev.key1_interrupts= %d, keydev.key1_gpios=%d\n",
           keydev.key1_interrupts,
           keydev.key1_gpios);
    gpio_direction_input(keydev.key1_gpios);
    keydev.key2_interrupts = of_irq_get(key_node, 1);
    keydev.key2_gpios      = of_get_named_gpio(key_node, "keydev_gpios", 1);
    printk("keydev.key2_interrupts= %d, keydev.key2_gpios=%d\n",
           keydev.key2_interrupts,
           keydev.key2_gpios);
    gpio_direction_input(keydev.key2_gpios);
    keydev.key3_interrupts = of_irq_get(key_node, 2);
    keydev.key3_gpios      = of_get_named_gpio(key_node, "keydev_gpios", 2);
    gpio_direction_input(keydev.key3_gpios);
    printk("keydev.key3_interrupts= %d, keydev.key3_gpios=%d\n",
           keydev.key3_interrupts,
           keydev.key3_gpios);

    //TODO 下降沿中断没有请求 请求中断并注册中断处理程序的函数;
    ret1 = request_irq(keydev.key1_interrupts,
                       key_ISR,
                       IRQF_TRIGGER_RISING | IRQF_TRIGGER_RISING,
                       "key-interrupts",
                       NULL);
    ret2 = request_irq(keydev.key2_interrupts,
                       key_ISR,
                       IRQF_TRIGGER_RISING | IRQF_TRIGGER_RISING,
                       "key-interrupts",
                       NULL);
    ret3 = request_irq(keydev.key3_interrupts,
                       key_ISR,
                       IRQF_TRIGGER_RISING | IRQF_TRIGGER_RISING,
                       "key-interrupts",
                       NULL);
    printk("request_irq %d %d %d\n", ret1, ret2, ret3);
    //对tasklet进行初始化：
    tasklet_setup(&keydev.key_tasklet, tasklet_function);

    return 0;
error:
    return -1;
}

int chrdev_driver_remove(struct platform_device* pdev)
{
    printk("啊哦，要断开了\n");

    tasklet_kill(&keydev.key_tasklet);

    free_irq(keydev.key1_interrupts, NULL);
    free_irq(keydev.key2_interrupts, NULL);
    free_irq(keydev.key3_interrupts, NULL);

    gpio_free(dts_led.led1);
    gpio_free(dts_led.led2);
    gpio_free(dts_led.led3);

    device_destroy(dts_led.class, dts_led.c_dev->dev);
    class_destroy(dts_led.class);

    cdev_del(dts_led.c_dev); //从内核中移除cdev，但是资源依然被占用
    unregister_chrdev_region(dts_led.c_dev->dev, 1);

    kfree(dts_led.c_dev);

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
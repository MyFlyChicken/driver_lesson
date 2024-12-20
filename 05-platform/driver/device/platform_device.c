#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>

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

/**
 * 查找不同类型时，不同类型的索引不进行累加
 * 如IORESOURCE_MEM，第二个为ioddr
 * IORESOURCE_IRQ，第一个为iodr
 */
struct resource my_device_001_res[] = {
    [0] =
        {
             .start = GPIO1_A4_IOMUX,
             .end   = GPIO1_A4_IOMUX + 4,
             .name  = "iomux",
             .flags = IORESOURCE_MEM,
             .desc  = GPIO1_A4_IOMUX,
             },
    [1] =
        {
             .start = GPIO1_A4_P,
             .end   = GPIO1_A4_P + 4,
             .name  = "iop",
             .flags = IORESOURCE_MEM,
             .desc  = GPIO1_A4_P_OFFSET,
             },
    [2] =
        {
             .start = GPIOE_MODR,
             .end   = GPIOE_MODR + 4,
             .name  = "ioddr",
             .flags = IORESOURCE_MEM,
             .desc  = GPIOE_MODR,
             },
    [3] =
        {
             .start = GPIOE_ODR,
             .end   = GPIOE_ODR + 4,
             .name  = "iodr",
             .flags = IORESOURCE_MEM,
             .desc  = GPIOE_ODR,
             },
    [4] =
        {
             .start = 66,
             .end   = 66,
             .flags = IORESOURCE_IRQ,
             .desc  = 77,
             },
};

//dev中的release方法：
void my_device_001_release(struct device* dev)
{
    printk("my_device_001_release执行了\n");
}

//定义一个平台设备
struct platform_device my_platform_device = {
    .name          = "yyf,device01",
    .id            = -1, //自动分配，或者给-1;
    .resource      = my_device_001_res,
    .num_resources = 5,
    .dev           = {
                      .release = my_device_001_release,
                      }
};

//入口函数：
int __init my_test_module_init(void)
{
    int ret = 0;
    //注册：
    ret = platform_device_register(&my_platform_device);
    if (ret < 0) {
        return -1;
    }
    return 0;
}

//出口函数：
void __exit my_test_module_exit(void)
{
    //注销：
    platform_device_unregister(&my_platform_device);
}

//指定许可：
MODULE_LICENSE("GPL");
MODULE_AUTHOR("gaowanxi, email:gaonetcom@163.com");
//指定入口及出口函数：
module_init(my_test_module_init);
module_exit(my_test_module_exit);
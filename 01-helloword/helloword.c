/* 包含头文件 */
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

/* 入口函数 */
static int __init hello_init(void)
{
    printk("Hello word\n");
    return 0;
}

/* 出口函数 */
static void __exit hello_exit(void)
{
    printk("Bye bye\n");
}

/* 指定模块名 */
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("YYF");

/* 指定模块入口，出口函数 */
module_init(hello_init);
module_exit(hello_exit);


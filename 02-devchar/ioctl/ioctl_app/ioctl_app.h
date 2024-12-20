#ifndef __ioctl_app_H_ //shift+U转换为大写
#define __ioctl_app_H_

#ifdef __cplusplus
extern "C" {
#endif

enum LED_STATUS
{
    LED1_ON  = 0,
    LED1_OFF = 1,
    LED2_ON  = 2,
    LED2_OFF = 3,
    //...
};
////封装命令码:
#define LED_CTL_CMD _IOW('L', 0, enum LED_STATUS)
#define LED_GTE_CMD _IOR('L', 1, enum LED_STATUS)

#ifdef __cplusplus
}
#endif

#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include "ioctl_app.h"

int main(int argc, char const* argv[])
{
    int fd = open("/dev/devchar", O_RDWR);
    if (fd == -1) {
        perror("open err");
        return -1;
    }
    char            buf[128] = {0};
    int             ret      = 0;
    enum LED_STATUS status;
    while (true) {
        fgets(buf, sizeof(buf), stdin);
        switch (buf[0]) {
        case '1': //LED1开灯
            status = LED1_ON;
            ret    = ioctl(fd, LED_CTL_CMD, &status);
            if (ret == -1) {
                perror("ioctol err:");
                return -1;
            }
            break;
        case '2': //LED1开灯
            status = LED1_OFF;
            ret    = ioctl(fd, LED_CTL_CMD, &status);
            if (ret == -1) {
                perror("ioctol err:");
                return -1;
            }
            break;
        //...更复杂的操作
        default:
            break;
        }
    }

    close(fd);

    return 0;
}

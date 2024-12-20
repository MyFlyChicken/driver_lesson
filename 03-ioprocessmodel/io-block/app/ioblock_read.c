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
    int             nbytes   = 0;
    enum LED_STATUS status;
    while (true) {
        memset(buf, 0, sizeof(buf));
        printf("开始读取\n");
        nbytes = read(fd, buf, sizeof(buf) - 1);
        if (nbytes == -1) {
            perror("read err:");
            return -1;
        }
        printf("读取的数据=%s\n", buf);
        printf("用户空间的当前进程号=%d\n", getpid());
    }

    close(fd);

    return 0;
}

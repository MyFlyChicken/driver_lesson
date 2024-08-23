// TODO 等待实现
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <linux/input.h>
#include <sys/signal.h>

int xxx_saple_dev_fd = 0;

// 信号的回调函数：
void sig_callback(int sig)
{
    if (sig == SIGIO) {
        char buf[128] = {0};
        int  nbytes   = read(xxx_saple_dev_fd, buf, sizeof(buf) - 1);
        if (nbytes == -1) {
            perror("read err:");
            return;
        }
        printf("接收到的数据为：%s\n", buf);
    }
}

int main(int argc, char const* argv[])
{
    xxx_saple_dev_fd = open("/dev/devchar", O_RDWR);
    // fd1 = open("/dev/input/mouse0", O_RDWR);
    if (xxx_saple_dev_fd == -1) {
        perror("open err");
        return -1;
    }

    // 1.建立SIGIO与回调处理函数的关系。
    signal(SIGIO, sig_callback);

    // 2.指定处理fd进行IO操作的进程：
    fcntl(xxx_saple_dev_fd, F_SETOWN, getpid());

    // 3.指定处理IO的方式：异步处理：
    int flags  = fcntl(xxx_saple_dev_fd, F_GETFL);
    flags     |= FASYNC;
    fcntl(xxx_saple_dev_fd, F_SETFL, flags);

    // 进程处理其它任务
    while (true) {
        sleep(1);
    }

    close(xxx_saple_dev_fd);

    return 0;
}

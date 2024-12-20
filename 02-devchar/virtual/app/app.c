#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
int main(int argc, char const* argv[])
{
    int fd = open("/dev/devchar", O_RDWR);
    if (fd == -1) {
        perror("open err");
        return -1;
    }

//0无限次
#define RUN_TIMES 0
    char buf[128] = {0};
#if (1 == RUN_TIMES)

    fgets(buf, sizeof(buf), stdin);
    int nbytes = write(fd, buf, strlen(buf));
    if (nbytes == -1) {
        perror("write err：\n");
        return -1;
    }
    memset(buf, 0, sizeof(buf));
    nbytes = read(fd, buf, sizeof(buf) - 1);
    if (nbytes == -1) {
        perror("read err:\n");
        return -1;
    }
    printf("接收的数据为：%s\n", buf);

#elif (0 == RUN_TIMES)
    while (true) {
        fgets(buf, sizeof(buf), stdin);
        buf[strlen(buf) - 1] = '\0';
        int nbytes           = write(fd, buf, strlen(buf));
        if (nbytes == -1) {
            perror("write err:\n");
            return -1;
        }
        memset(buf, 0, sizeof(buf));
        nbytes = read(fd, buf, sizeof(buf) - 1);
        if (nbytes == -1) {
            perror("read err:\n");
            return -1;
        }
        printf("接收的数据为：%s\n", buf);
    }
#endif
    close(fd);

    return 0;
}

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
int main(int argc, char const *argv[])
{
    int fd = open("/dev/devchar",O_RDWR); 
    if(fd == -1)
    {
        perror("open err");
        return -1;
    }
    char buf[128] = {0};
    fgets(buf,sizeof(buf),stdin);
    int nbytes = write(fd,buf,strlen(buf));
    if(nbytes == -1)
    {
        perror("write err：\n");
        return -1;
    }
    memset(buf,0, sizeof(buf));
    nbytes = read(fd,buf,sizeof(buf)-1);
    if(nbytes == -1)
    {
        perror("read err:\n");
        return -1;
    }
    printf("接收的数据为：%s\n",buf);

    close(fd);

    return 0;
}

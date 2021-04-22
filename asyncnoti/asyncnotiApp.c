#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>
/*
 *argc:应用程序参数个数
 *argv[]:具体的参数内容，字符串形式 
 *./imx6uirqAPP  <filename>  
 * ./imx6uirqAPP /dev/mx6uirq
 */
int fd;

static void sigio_signal_func(int num)
{
    int err;
    unsigned int keyvalue = 0;
    read(fd,keyvalue,sizeof(keyvalue));
    if(err < 0) {

    } else {
        printf("sigio signal! key value = %d\r\n",keyvalue);
    }
}

int main(int argc, char *argv[])
{
    int ret;
    char *filename;
    unsigned char data;
    int flags = 0;

    if(argc != 2) {
        printf("Error Usage!\r\n");
        return -1;
    }

    filename = argv[1];

    fd = open(filename, O_RDWR);
    if(fd < 0) {
        printf("file %s open failed!\r\n", filename);
        return -1;
    }

    /* 循环读取 */
    while(1) {
        ret = read(fd, &data, sizeof(data));
        if(ret < 0) {

        } else {
            if(data)
                printf("key value = %#x\r\n", data);
        }

    }

    signal(SIGIO,sigio_signal_func);

    fcntl(fd, F_SETOWN,getpid());//设置当前进程接受sigio信号
    flags = fcntl(fd,F_GETFL);
    fcntl(fd, F_SETFL,flags | FASYNC);//异步通知

    while(1){
        sleep(2);
    }

    close(fd);

    return 0;
}

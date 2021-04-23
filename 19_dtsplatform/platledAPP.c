#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    int fd,retvalue;
    char *filename;
    unsigned char databuf[1];

    if(argc != 3){
        printf("Error usage!\r\n");
        return -1;
    };

    filename = argv[1];

    fd = open(argv[1], O_RDWR);
    if(fd < 0){
        printf("file %s open failed!\r\n",filename);
        return -1;
    };

    databuf[0] = atoi(argv[2]);  //字符转换成数字
    retvalue = write(fd, databuf,sizeof(databuf));
    if(retvalue < 0){
        printf("LED Control Failed!\r\n");
        close(fd);
        return -1;
    }
    close(fd);

    return 0;
}
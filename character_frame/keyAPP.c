#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define KEY0VALUE 0xF0
#define INVAKEY   0x00

int main(int argc, char *argv[])
{
    int fd,retvalue,value;
    int cnt = 0;
    char *filename;
    unsigned char databuf[1];

    if(argc != 2){
        printf("Error usage!\r\n");
        return -1;
    };

    filename = argv[1];

    fd = open(argv[1], O_RDWR);
    if(fd < 0){
        printf("file %s open failed!\r\n",filename);
        return -1;
    };

    /*循环读取按键值*/
    while(1){
        read(fd,&value,sizeof(value));
        if(value == KEY0VALUE){
            printf("KEY0 Press, value = %d\r\n",value);
        }
    }

    close(fd);

    return 0;
}

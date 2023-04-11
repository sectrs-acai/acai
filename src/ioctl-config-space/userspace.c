/***************************************************************************//**
*  \file       test_app.c
*
*  \details    Userspace application to test the Device driver
*
*  \author     EmbeTronicX
*
*  \Tested with Linux raspberrypi 5.10.27-v7l-embetronicx-custom+
*
*******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include<sys/ioctl.h>
#include <stdint.h>
 
#define WR_VALUE_32 _IOW('a', 'a', uint32_t *)
#define WR_VALUE_16 _IOW('a', 'b', uint16_t *)
#define WR_VALUE_8 _IOW('a', 'c', uint8_t *)

#define RD_VALUE_32 _IOWR('a', 'd', uint32_t *)
#define RD_VALUE_16 _IOWR('a', 'e', uint16_t *)
#define RD_VALUE_8 _IOWR('a', 'f', uint8_t *)

struct ioctl_argument {
    uint64_t offset;
    uint64_t data;
};

int main()
{
        int fd;
        int32_t value, number;
        struct ioctl_argument call_data;
        printf("*********************************\n");
        printf("*******WWW.EmbeTronicX.com*******\n");
 
        printf("\nOpening Driver\n");
        fd = open("/dev/etx_device", O_RDWR);
        if(fd < 0) {
                printf("Cannot open device file...\n");
                return 0;
        }
 
        printf("Enter the Value to send\n");
        scanf("%d",&number);
        printf("Writing Value to Driver\n");
        call_data.data = number;
        call_data.offset = 0x20;
        ioctl(fd, WR_VALUE_32, &call_data); 

        call_data.data = 0;
        call_data.offset = 0x20;
        ioctl(fd, RD_VALUE_32, &call_data); 
        printf("Got result back from ioctl %ld\n",call_data.data);

        call_data.data = 0;
        call_data.offset = 0x20;
        ioctl(fd, RD_VALUE_16, &call_data); 
        printf("Got result back from ioctl %ld\n",call_data.data);
 
        call_data.data = 0;
        call_data.offset = 0x20;
        ioctl(fd, RD_VALUE_8, &call_data); 
        printf("Got result back from ioctl %ld\n",call_data.data);
        
        printf("Closing Driver\n");
        close(fd);
}
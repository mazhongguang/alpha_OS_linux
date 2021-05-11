#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "linux/ioctl.h"

#define KEY0VALUE 0XF0
#define INVAKEY 0X00

#define CLOSE_CMD _IO(0XEF, 1)
#define OPEN_CMD _IO(0XEF, 2)
#define SETPERIOD_CMD _IOW(0XEF, 3, int)

/* @brief 
 * @param argc 应用程序参数个数
 * @param argv[] 具体的参数内容，字符串形式
 * ./irqapp <filename> 
 * ./irqapp /dev/irq
 */
int main(int argc, char *argv[])
{
	int fd = 0, ret;
	char *filename;
	unsigned char data;

	if(argc != 2)
	{
		printf("error usage!\r\n");
		return -1;
	}

	filename = argv[1];
	fd = open(filename, O_RDWR);
	if (fd < 0)
	{
		printf(" file %s open failed !\r\n", filename);
		return -1;
	}

	/* 循环读取 */
	while (1)
	{
		ret = read(fd, &data, sizeof(data));
		if (ret < 0)
		{
		}
		else
		{
			printf("key value = %#x\r\n", data);
		}
	}

	close(fd);

	return 0;
}

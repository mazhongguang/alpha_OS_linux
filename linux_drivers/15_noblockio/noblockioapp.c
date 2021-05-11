#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "linux/ioctl.h"
#include "linux/types.h"
#include <sys/time.h>
#include <sys/types.h>
#include <sys/select.h>
#include <poll.h>

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
	/*fd_set readfds;*/
	/*struct timeval timeout;*/
	struct pollfd fds;

	int fd = 0, ret;
	char *filename;
	unsigned char data;

	if(argc != 2)
	{
		printf("error usage!\r\n");
		return -1;
	}

	filename = argv[1];
	fd = open(filename, O_RDWR | O_NONBLOCK);	/* 非阻塞方式打开 */
	if (fd < 0)
	{
		printf(" file %s open failed !\r\n", filename);
		return -1;
	}

	/* 循环读取 */
	while (1)
	{
#if 0
		FD_ZERO(&readfds);
		FD_SET(fd, &readfds);

		timeout.tv_sec = 0;
		timeout.tv_usec = 500000;	/* 500ms */
		ret = select(fd + 1,&readfds, NULL, NULL, &timeout);
		switch (ret)
		{
			case 0:	/* 超时 */
				printf("timeout \r\n");
				break;
			case -1:	/* 错误 */
				printf("error!\r\n");
				break;
			default:	/* 可以读取数据 */
				if (FD_ISSET(fd, &readfds))	/* 判断是否为fd文件描述符 */
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
				break;
		}
#endif
		fds.fd = fd;
		fds.events = POLLIN;
		ret = poll(&fds, 1, 500);	/* 超时500ms */
		if (ret == 0)	/* 超时 */
		{
			printf("timeout!\r\n");
		}
		else if (ret < 0)	/* error */
		{
			printf("error!\r\n");
		}
		else	/* 可以读取 */
		{
			if (fds.revents | POLLIN)	/* 可读取 */
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
		}
	}

	close(fd);

	return 0;
}

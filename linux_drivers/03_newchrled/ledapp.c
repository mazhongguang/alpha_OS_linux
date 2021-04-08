#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define LEDON 1
#define LEDOFF 0

/* @brief 
 * @param argc 应用程序参数个数
 * @param argv[] 具体的参数内容，字符串形式
 * ./app <filename> <0:1> 0->led off,1-> LED ON
 * ./app /dev/led 0 led off 
 * ./app /dev/led 1 led on
 */
int main(int argc, char *argv[])
{
	int ret = 0;
	int fd = 0;
	char *filename;
	unsigned char *databuf[1];

	if(argc != 3)
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

	/* 字符形式转换为数字形式 */
	databuf[0] = atoi(argv[2]);
	ret = write(fd, databuf, sizeof(databuf));
	if (ret < 0)
	{
		printf("led control failed~\r\n");
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}

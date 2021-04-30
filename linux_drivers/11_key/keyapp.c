#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#define KEY0VALUE 0XF0
#define INVAKEY 0X00
/* @brief 
 * @param argc 应用程序参数个数
 * @param argv[] 具体的参数内容，字符串形式
 * ./keyapp <filename> 
 * ./keyapp /dev/key
 */
int main(int argc, char *argv[])
{
	int ret = 0;
	int fd = 0, value;
	char *filename;
	unsigned char databuf[1];

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
		read(fd, &value, sizeof(value));
		if (value == KEY0VALUE)
		{
			printf("KEY0 Press ,value = %d\r\n", value);
		}
	}

	close(fd);

	return 0;
}

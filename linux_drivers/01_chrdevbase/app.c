#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

/* @brief 
 * @param argc 应用程序参数个数
 * @param argv[] 具体的参数内容，字符串形式
 * ./app <filename> <1:2> 1->read,2->write
 * ./app /dev/chrdevbase 1 从驱动里面读数据
 * ./app /dev/chrdevbase 2 向驱动里面写数据
 */
int main(int argc, char *argv[])
{
	int ret = 0;
	int fd = 0;
	char *filename;
	char readbuf[100], writebuf[100];
	static char usrdata[] = {"usr data"};

	if(argc !=3)
	{
		printf("error usage!\r\n");
		return -1;
	}

	filename = argv[1];
	fd = open(filename, O_RDWR);
	if (fd < 0)
	{
		printf("can't open file %s\r\n", filename);
		return -1;
	}

	if (atoi(argv[2]) == 1)
	{
		ret = read(fd, readbuf, 50);
		if (ret < 0)
		{
			printf("read file %s faliled!\r\n", filename);
			return -1;
		}
		else
		{
			printf("APP read data:%s\r\n", readbuf);
		}
	}

	if (atoi(argv[2]) == 2)
	{
		memcpy(writebuf, usrdata, sizeof(usrdata));
		ret = write(fd, writebuf, 50);
		if (ret < 0)
		{
			printf("write file %s failed!\r\n", filename);
			return -1;
		}
		else
		{
		}
	}

	ret = close(fd);
	if (ret < 0)
	{
		printf("close file %s failed!\r\n", filename);
		return -1;
	}
	else
	{
	}

	return 0;
}

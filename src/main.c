#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

int main(int argc, char **argv)
{
	if(argc < 2){
		return 1;
	}

	int fd = open(argv[1], O_CREAT|O_TRUNC|O_RDWR, 0777);
	if(-1 == fd){
		fprintf(stderr, "open: %d %s\n", errno, strerror(errno));
		return 1;
	}
	uint8_t data[] = "hello";
	ssize_t s = write(fd, data, sizeof(data));
	if(0 == s){
		fprintf(stderr, "write: %d %s\n", errno, strerror(errno));
	}
	close(fd);
	return 0;
}


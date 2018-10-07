#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define FIFO_CLEAR 0x01
#define BUFFER_LEN 20

void main(void)
{
	int fd, num;
	char rd_ch[BUFFER_LEN];
	fd_set rfds;
	struct timeval tv;

	fd = open("/dev/globalmem", O_RDONLY | O_NONBLOCK);

	if (fd != -1){
		if(ioctl(fd, FIFO_CLEAR, 0) < 0)
			printf("ioctl command failed\n");

		while(1){
			FD_ZERO(&rfds);
//			FD_ZERO(&wfds);
			FD_SET(fd, &rfds);
//			FD_SET(fd, &wfds);
			tv.tv_sec = 5;
			tv.tv_usec = 0;
			select(fd+1, &rfds, NULL, NULL, &tv);
			
			if(FD_ISSET(fd, &rfds))
				printf("Poll monitor: can be read\n");
		/*	if(FD_ISSET(fd, &wfds))
				printf("Poll monitor: can be written\n");
		*/
		}
	}else
		printf("device open failure\n");
}

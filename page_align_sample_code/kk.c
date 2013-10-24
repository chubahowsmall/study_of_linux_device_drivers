#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>


#define FBDEV	"/dev/fb0"
#define PAGE_ALIGN(addr, page_size, page_mask)    (((addr)+page_size-1) & page_mask)


int main(int argc, char *argv[])
{
	int ret = 0;
	int fd = (-1);
	struct fb_var_screeninfo fb_var;
	struct fb_fix_screeninfo fb_fix;
	int bufLength = 0;
	int pageSize = 0;
	int pageMask = 0;
#if 0
	fd = open(FBDEV, O_RDWR);
	if(fd < 0)
	{
		printf("Open %s fail\n", FBDEV);
		exit(-1);
	}

	memset(&fb_var, 0, sizeof(fb_var));
	memset(&fb_fix, 0, sizeof(fb_fix));
	if(ioctl(fd, FBIOGET_VSCREENINFO, &fb_var))
	{
		printf("FBIOGET_VSCREENINFO fail\n");
		close(fd);
		exit(-1);
	}
	else if(ioctl(fd, FBIOGET_FSCREENINFO, &fb_fix))
	{
		printf("FBIOGET_FSCREENINFO fail\n");
		close(fd);
		exit(-1);
	}

	pageSize = sysconf(_SC_PAGESIZE);
	pageMask = ~(pageSize - 1);

	bufLength = fb_fix.smem_len;
#else
	pageSize = sysconf(_SC_PAGESIZE);
	pageMask = ~(pageSize - 1);

	printf("0x22000001 aligned = %#x\n", PAGE_ALIGN(0x22000001, pageSize, pageMask));
#endif



	return ret;
}

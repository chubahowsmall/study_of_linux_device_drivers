#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include "../spca6350_reg_mapper.h"

int main(int argc, char **argv)
{
	int fd = 0;
	int ret = 0;
	unsigned long addr = 0;
	unsigned long offset = 0x64;
	struct _regMapper reg;
	unsigned long val_from_mmap;
	unsigned long val_from_ioctl;


	fd = open("/dev/reg_mapper", O_RDWR);
	if(fd <= 0)
	{
		printf("Open /dev/reg_mapper failed\n");
		exit(1);
	}

	addr = (unsigned long)  mmap (0, 0x10000/* 64KBytes */, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(NULL == (void*)addr)
	{
		close(fd);
		printf("%s: mmap() failed\n", __func__);
		exit(2);
	}
	else printf("%s: mapped addr@%#x\n", __func__, addr);

	printf("%s: read %#x=%#x\n", __func__, (addr + offset), *((unsigned long *)(addr + offset)));
	val_from_mmap = *((unsigned long*)(addr + offset));


	/*
		This is using the kseg1 address to access specific register.
		In this example, we read the offset 0x64 from the starting address of spca6350 registers (0xb0000000), and
		it's the same as the mmap() example above.
		We can check the value read from mmap() and ioctl(), they are the same.
	*/
	reg.addr = 0xb0000000 + offset;
	reg.val = 0;
	ret = ioctl(fd, REG_MAPPER_READ, &reg);
	printf("%s: ret = %#x, reg.addr=%#x, reg.val=%#x\n", __func__, ret, reg.addr, reg.val);

	val_from_ioctl = reg.val;

	printf("The values are %s\n", (val_from_ioctl == val_from_mmap)?"the same":"not the same");


	munmap((void*)addr, 0x10000);

	close(fd);

	return 0;
}

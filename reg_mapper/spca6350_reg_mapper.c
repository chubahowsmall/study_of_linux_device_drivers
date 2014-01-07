#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/ioport.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include "spca6350_reg_mapper.h"
#include "common.h"

/*
	MIPS memory map contains 4 sections (refer to "See MIPS Run")
	kuseg: 0x00000000 ~ 0x7FFFFFFF 2GB 
			user process virtual address, mapped and cached
			this area can be touched only MMU enabled. 
			if a user program wants to access address with the significant bit set to 1, an exception will raise.
			
	kseg0: 0x80000000 ~ 0x9FFFFFFF 512MB 
			unmapped, cached, directely access physical address
			these addresses can be translated into physical address by stripping of the top bit and mapping
			them contiguously into the low 512MB of physical memory
			OS kernel which uses MMU will be put here.

	kseg1: 0xA0000000 ~ 0xBFFFFFFF 512MB 
			unmapped, uncached, directely access physical address
			these address can be translated into physical address by stripping of the leading 3 bits, and mapping
			them contiguously into the low 512MB of physical memory

			when system reset, this area is guaranteed to behave properly.
			when system reset, address of 0xBFC00000 will be the entry point of the system.
			0xBFC00000 can map to physical address of 0x1FC00000.
			
	kseg2: 0xC0000000 ~ 0xFFFFFFFF 1GB 
			kernel virtual address, mapped and cached
			this area can only be accessed in kernel mode with MMU enabled.
			Don't access this area before MMU is enabled.
			

	virtual memory conversion

	31 | 30 | 29 | ....
	---|---|---|---
	 0  | x  | x   | .... kuseg
	 1  | 0  | 0   | .... kseg0
	 1  | 0  | 1  | .... kseg1
	 1  | 1  | x  | .... kseg2

*/


/*
 * address space conversion and alignment.
 */
#define BADDR_CACHE_BASE		0x80000000
#define BADDR_UNCACHE_BASE		0xa0000000
#define BADDR_ROM_BASE			0x9fc00000

/* logical cached byte address <=> logical uncached byte address. */
#define LOGI_CACH_BADDR_TO_LOGI_UNCACH_BADDR(addr) \
	((void *)((UINT32)(addr) | 0x20000000))

#define LOGI_UNCACH_BADDR_TO_LOGI_CACH_BADDR(addr) \
	((void *)((UINT32)(addr) & ~0x20000000))

/* logical cached byte address <=> physical byte address. */
#define LOGI_CACH_BADDR_TO_PHY_BADDR(addr) \
	((UINT32)(addr) & ~BADDR_CACHE_BASE)

#define PHY_BADDR_TO_LOGI_CACH_BADDR(addr) \
	((void *)((UINT32)(addr) | BADDR_CACHE_BASE))

/* logical uncached byte address <=> physical byte address. */
#define LOGI_UNCACH_BADDR_TO_PHY_BADDR(addr) \
	LOGI_CACH_BADDR_TO_PHY_BADDR(LOGI_UNCACH_BADDR_TO_LOGI_CACH_BADDR(addr))

#define PHY_BADDR_TO_LOGI_UNCACH_BADDR(addr) \
	LOGI_CACH_BADDR_TO_LOGI_UNCACH_BADDR(PHY_BADDR_TO_LOGI_CACH_BADDR(addr))

/* logical uncached byte address <=> physical word address. */
#define LOGI_UNCACH_BADDR_TO_PHY_WADDR(addr) \
	((UINT32)LOGI_UNCACH_BADDR_TO_PHY_BADDR(addr) >> 1)

#define PHY_WADDR_TO_LOGI_UNCACH_BADDR(addr) \
	PHY_BADDR_TO_LOGI_UNCACH_BADDR((UINT32)(addr) << 1)

/* logical cached byte address <=> physical word address. */
#define LOGI_CACH_BADDR_TO_PHY_WADDR(addr) \
	((UINT32)LOGI_CACH_BADDR_TO_PHY_BADDR(addr) >> 1)

#define PHY_WADDR_TO_LOGI_CACH_BADDR(addr) \
	PHY_BADDR_TO_LOGI_CACH_BADDR((UINT32)(addr) << 1)


#define MEM_MAX_OPEN_COUNT	1

#define TEST_REG_BASE_LOGI	0xb0000000
#define TEST_REG_BASE_PHYS	(LOGI_UNCACH_BADDR_TO_PHY_BADDR(TEST_REG_BASE_LOGI))

extern void *first_unused_virt_memory;
extern void *first_unused_phy_memory;
extern unsigned long v35_mem_size;

static dev_t spca6350_mem_dev_t;
static struct cdev *mem_cdev;
static unsigned int current_open_count;


static int reg_mapper_ioctl(struct inode *inode, struct file *filp,
			unsigned int cmd, unsigned long arg)
{
	int err = 0;
	int ret = 0;
	struct _regMapper reg;
	unsigned long val = 0;

	if((inode == NULL) || (filp == NULL))
		return -EFAULT;
	if(_IOC_TYPE(cmd) != SPCA6350_REG_MAPPER_IOC_MAGIC)
		return -ENOTTY;
	if(_IOC_NR(cmd) > SPCA6350_MEM_MAXNR)
		return -ENOTTY;
#if 0	
	if(_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void *) arg, _IOC_SIZE(cmd));
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void *) arg, _IOC_SIZE(cmd));
	if(err)
		return -EFAULT;
#endif

	printk("%s:\n", __func__);
	switch (cmd)
	{
		case REG_MAPPER_READ:
			
			copy_from_user(&reg, (struct _regMapper __user *) arg, sizeof(reg)); 
			printk("%s: read reg%#x\n", __func__, reg.addr);
			reg.val = readl((unsigned long*)reg.addr);
			ret = reg.val;
			printk("%s: val=%#x\n",__func__, ret);
			copy_to_user((struct _regMapper __user *)arg, &reg, sizeof(reg));
			break;
		case REG_MAPPER_WRITE:
			ret = copy_from_user(&reg, (struct _regMapper __user *)arg, sizeof(reg));
			if(ret == 0)
				writel((unsigned long*)reg.addr, reg.val);

			break;
		default:
			break;
	}

	return ret;
}


static int reg_mapper_open(struct inode *inode, struct file *filp)
{
	if(current_open_count == MEM_MAX_OPEN_COUNT)
	{
		printk("%s: opened too many times\n", __func__);
		return -EBUSY;
	}
	

	current_open_count += 1;

	printk("%s: current_open_count = %u\n", __func__, current_open_count);

	return 0;
}


static int reg_mapper_close(struct inode *inode, struct file *filp)
{
	current_open_count -= 1;
	/*free_bootmem((unsigned long)first_unused_memory, v35_mem_size);*/
	
	return 0;
}



static int reg_mapper_mmap(struct file *filp , struct vm_area_struct *vma)
{

	printk("%s: vma->vm_start=%#x\n", __func__, vma->vm_start);
	printk("%s: size = %p\n", __func__, vma->vm_end - vma->vm_start);
	
	/* calculate phys page-offset */
	printk("%s: phys=%#x\n", __func__, TEST_REG_BASE_PHYS);
	vma->vm_pgoff = (TEST_REG_BASE_PHYS >> PAGE_SHIFT);
	/* For noncached mem area */
	vma->vm_page_prot = pgprot_noncached( vma->vm_page_prot ) ;
	remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, (vma->vm_end - vma->vm_start), vma->vm_page_prot) ;
	
	return 0 ;
}



static ssize_t expose_first_unused_virt_memory(struct class *class, char *buf)
{
	if(NULL == v35_reserved_memory_addr_get())
		sprintf(buf, "0\n");
	else sprintf(buf, "%u\n", (unsigned int)v35_reserved_memory_addr_get());

	return strlen(buf);
}

static ssize_t expose_first_unused_phy_memory(struct class *class, char *buf)
{
	if(NULL == v35_reserved_memory_addr_get())
		sprintf(buf, "0\n");
	else sprintf(buf, "%u\n", virt_to_phys(v35_reserved_memory_addr_get()));

	return strlen(buf);
}

static ssize_t expose_mem_size(struct class *class, char *buf)
{
	
	sprintf(buf, "%u\n", v35_reserved_memory_size_get());

	return strlen(buf);
}
struct class *spca6350_reg_mapper_class = NULL;

static struct class_attribute spca6350_mem_virt_addr_attr = {

	.show = expose_first_unused_virt_memory,
	.store = NULL,
};
static struct class_attribute spca6350_mem_phy_addr_attr = {

	.show = expose_first_unused_phy_memory,
	.store = NULL,
};
static struct class_attribute spca6350_mem_size_attr = {

	.show = expose_mem_size,
	.store = NULL,
};


static struct file_operations mem_fops = {

	.open		= reg_mapper_open,
	.release	= reg_mapper_close,
	.ioctl		= reg_mapper_ioctl,
	.mmap		= reg_mapper_mmap,

};


int __init spca6350_reg_mapper_init(void)
{
	int ret = 0;	

	spca6350_reg_mapper_class = class_create(THIS_MODULE, "spca6350_register_mapper ");

	spca6350_mem_virt_addr_attr.attr.name = "first_unused_virt_memory";
	spca6350_mem_virt_addr_attr.attr.owner = THIS_MODULE;
	spca6350_mem_virt_addr_attr.attr.mode = S_IRUGO;

	spca6350_mem_phy_addr_attr.attr.name = "first_unused_phy_memory";
	spca6350_mem_phy_addr_attr.attr.owner = THIS_MODULE;
	spca6350_mem_phy_addr_attr.attr.mode = S_IRUGO;

	spca6350_mem_size_attr.attr.name = "size";
	spca6350_mem_size_attr.attr.owner = THIS_MODULE;
	spca6350_mem_size_attr.attr.mode = S_IRUGO;

	ret = class_create_file(spca6350_reg_mapper_class, &spca6350_mem_virt_addr_attr);
	ret = class_create_file(spca6350_reg_mapper_class, &spca6350_mem_phy_addr_attr);
	ret = class_create_file(spca6350_reg_mapper_class, &spca6350_mem_size_attr);

	/* create and register device number */
	ret = alloc_chrdev_region(&spca6350_mem_dev_t, 0, 1, "spca6350_reg_mapper");
	if(ret != 0)
	{
		printk("%s: device number allocation fail\n", __func__);
		return -EINVAL;
	}

	/* create cdev instance */
	mem_cdev = cdev_alloc();
	cdev_init(mem_cdev, &mem_fops);
	mem_cdev->owner = THIS_MODULE;
	mem_cdev->ops = &mem_fops;
	ret = cdev_add(mem_cdev, spca6350_mem_dev_t, 1);
	if(ret != 0)
	{	
		printk("%s: cdev_add fail\n", __func__);
		return ret;
	}

	device_create(spca6350_reg_mapper_class, NULL, MKDEV(MAJOR(spca6350_mem_dev_t), 0), NULL, "reg_mapper");

	current_open_count = 0;

	printk("%s: REG_BASE_LOGI=%#x\n", __func__, TEST_REG_BASE_LOGI);
	printk("%s: REG_BASE_PHYS=%#x\n", __func__, TEST_REG_BASE_PHYS);

	return 0;
}

void __exit spca6350_reg_mapper_exit(void)
{
	printk("%s exit\n", __func__);

	unregister_chrdev_region(spca6350_mem_dev_t, 1);
	cdev_del(mem_cdev);
}

module_init(spca6350_reg_mapper_init);
module_exit(spca6350_reg_mapper_exit);

MODULE_AUTHOR("Nobody");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("MMAP Internal Registers to Userland");


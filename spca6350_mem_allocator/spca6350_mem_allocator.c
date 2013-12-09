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
#include "spca6350_mem_allocator.h"


#define MEM_MAX_OPEN_COUNT	2


extern void *first_unused_memory;
extern unsigned int v35_mem_size;

static dev_t spca6350_mem_dev_t;
static struct cdev *mem_cdev;
static unsigned int current_open_count;


static int mem_ioctl(struct inode *inode, struct file *filp,
			unsigned int cmd, unsigned long arg)
{
	int err = 0;
	int ret = 0;

	if((inode == NULL) || (filp == NULL))
		return -EFAULT;
	if(_IOC_TYPE(cmd) != SPCA6350_MEM_IOC_MAGIC)
		return -ENOTTY;
	if(_IOC_NR(cmd) > SPCA6350_MEM_MAXNR)
		return -ENOTTY;
	
	if(_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE, (void *) arg, _IOC_SIZE(cmd));
	else if(_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ, (void *) arg, _IOC_SIZE(cmd));
	if(err)
		return -EFAULT;

	switch (cmd)
	{
	}

	return ret;
}


static int mem_open(struct inode *inode, struct file *filp)
{
	if(current_open_count == MEM_MAX_OPEN_COUNT)
	{
		printk("%s: opened too many times\n", __func__);
		return -EINVAL;
	}

	current_open_count += 1;

	return 0;
}


static int mem_close(struct inode *inode, struct file *filp)
{
	current_open_count -= 1;

	return 0;
}



static int mem_mmap(struct file *filp , struct vm_area_struct *vma)
{
	unsigned long offset, phys, start ;
	
	if(NULL == first_unused_memory )
	{
		return -1;
	}

	offset  =  (vma->vm_pgoff << PAGE_SHIFT) ;
	phys    =  (unsigned long)first_unused_memory ;
	start   =  offset + phys ;
	vma->vm_pgoff = start >> PAGE_SHIFT ;
	
	/* For noncached mem area */
	vma->vm_page_prot = pgprot_noncached( vma->vm_page_prot ) ;
	remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, (vma->vm_end - vma->vm_start), vma->vm_page_prot) ;
	
	return 0 ;
}



static ssize_t expose_first_unused_memory(struct class *class, char *buf)
{
	
	sprintf(buf, "%#x\n", (unsigned int)first_unused_memory);

	return strlen(buf);
}

static ssize_t expose_mem_size(struct class *class, char *buf)
{
	
	sprintf(buf, "%#x\n", v35_mem_size);

	return strlen(buf);
}
struct class *spca6350_mem_allocator_class = NULL;

static struct class_attribute spca6350_mem_addr_attr = {

	.show = expose_first_unused_memory,
	.store = NULL,
};
static struct class_attribute spca6350_mem_size_attr = {

	.show = expose_mem_size,
	.store = NULL,
};


static struct file_operations mem_fops = {

	.open		= mem_open,
	.release	= mem_close,
	.ioctl		= mem_ioctl,
	.mmap		= mem_mmap,

};


int __init spca6350_mem_allocator_init(void)
{
	int ret = 0;	

	spca6350_mem_allocator_class = class_create(THIS_MODULE, "spca6350_mem_allocator");

	spca6350_mem_addr_attr.attr.name = "first_unused_byte";
	spca6350_mem_addr_attr.attr.owner = THIS_MODULE;
	spca6350_mem_addr_attr.attr.mode = S_IRUGO;

	spca6350_mem_size_attr.attr.name = "size";
	spca6350_mem_size_attr.attr.owner = THIS_MODULE;
	spca6350_mem_size_attr.attr.mode = S_IRUGO;

	ret = class_create_file(spca6350_mem_allocator_class, &spca6350_mem_addr_attr);
	ret = class_create_file(spca6350_mem_allocator_class, &spca6350_mem_size_attr);

	/* create and register device number */
	ret = alloc_chrdev_region(&spca6350_mem_dev_t, 0, 1, "spca6350_mem_allocator");
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

	device_create(spca6350_mem_allocator_class, NULL, MKDEV(MAJOR(spca6350_mem_dev_t), 0), NULL, "mem_allocator");

	current_open_count = 0;

	return 0;
}

void __exit spca6350_mem_allocator_exit(void)
{
	printk("%s exit\n", __func__);

	unregister_chrdev_region(spca6350_mem_dev_t, 1);
	cdev_del(mem_cdev);
}

module_init(spca6350_mem_allocator_init);
module_exit(spca6350_mem_allocator_exit);

MODULE_AUTHOR("Nobody");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Expose Memory Information");


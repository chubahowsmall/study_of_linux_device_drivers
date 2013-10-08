#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>


static int __init hello_init(void)
{
	printk("Hello, world.\n");
	return 0;
}

static void __exit hello_exit(void)
{
	printk("Bye.\n");
}

module_init(hello_init);
module_exit(hello_exit);

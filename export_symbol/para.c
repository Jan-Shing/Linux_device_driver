#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("Dual BSD/GPL");

static char *book_name = "baohua Linux Device Driver";
module_param(book_name, charp, S_IRUGO);

static int book_num = 4000;
/* kernel won't create node on /sys/modules/para/parameter for this parameter'*/
module_param(book_num, int, 0);

int add_integer(int, int);


static int __init hello_init(void)
{
	printk(KERN_INFO "book name: %s and a + b is %d\n", book_name, add_integer(10,20));
	printk(KERN_INFO "book num: %d\n", book_num);
	return 0;
}

static void __exit hello_exit(void)
{
	printk(KERN_ALERT "driver unloaded\n");
}

module_init(hello_init);
module_exit(hello_exit);

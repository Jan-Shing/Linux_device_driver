#include <linux/module.h>
#include <linux/init.h>

MODULE_LICENSE("Dual BSD/GPL");


/* you can see the symbol by grep integer /proc/kallsyms */
int add_integer(int a, int b)
{
	return a + b;
}
/*Licesnse with GPL v2 will using EXPORT_SYMBOL_GPL*/
EXPORT_SYMBOL(add_integer);

int sub_integer(int a, int b)
{
	return a - b;
}
EXPORT_SYMBOL(sub_integer);



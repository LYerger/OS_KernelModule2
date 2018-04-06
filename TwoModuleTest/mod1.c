// Small module to test compiling multiple modules in one makefile
// Try to make them share memory

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>

int commonInt = 10;
EXPORT_SYMBOL(commonInt);

int init_module(void) {
	printk(KERN_INFO "Installing mod1.c...\n");
	printk(KERN_INFO "Sending commonInt=%d to mod2.c\n", commonInt);
	return 0;
}

void cleanup_module(void) {
	printk(KERN_INFO "Removing mod1.c, commonInt now = %d...\n", commonInt);
}

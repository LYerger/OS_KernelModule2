// Small module to test compiling multiple modules in one makefile
// Try to make them share memory

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/kernel.h>

extern int commonInt;

int init_module(void) {
	printk(KERN_INFO "Installing mod2.c...\n");
	printk(KERN_INFO "Received commonInt=%d from mod1.c\n", commonInt);
	commonInt = 15;
	printk(KERN_INFO "Changed commonInt to %d in mod2.c\n", commonInt);
	return 0;
}

void cleanup_module(void) {
	printk(KERN_INFO "Removing mod2.c...\n");
}
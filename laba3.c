#include <linux/module.h>  
#include <linux/kernel.h>  

MODULE_LICENSE("GPL");

static int __init laba3_init(void) {
    printk(KERN_INFO "Welcome to the Tomsk State University\n");
    return 0;  
}

static void __exit laba3_exit(void) {
    printk(KERN_INFO "Tomsk State University forever!\n");
}

module_init(laba3_init);
module_exit(laba3_exit);

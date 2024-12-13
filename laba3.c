#include <linux/module.h>  // Для модулей ядра
#include <linux/kernel.h>  // Для printk и KERN_*

// Указание лицензии модуля
MODULE_LICENSE("GPL");

// Загрузка модуля
static int __init laba3_init(void) {
    printk(KERN_INFO "Welcome to the Tomsk State University\n");
    return 0;  // Возвращаем 0, если модуль загрузился успешно
}

// Выгрузка модуля
static void __exit laba3_exit(void) {
    printk(KERN_INFO "Tomsk State University forever!\n");
}

// Определяем функции загрузки и выгрузки модуля
module_init(laba3_init);
module_exit(laba3_exit);
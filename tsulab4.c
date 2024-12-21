#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

#define SCALE 1000000   // Масштаб
#define PI 3141592   // Значение Pi * SCALE (примерно 3.141592)
#define PROC_NAME "tsulab"

// модуль для long long int
static inline long long llabs(long long value) {
    return value < 0 ? -value : value;
}

static struct proc_dir_entry *proc_file;
static int current_term = 0;
static long long sum_scaled = 0; // Сумма ряда в масштабированном виде

static long long current_fact = 1; // факториал
static long long x_power = PI / 3; // x^

static int overflow_flag = 0; // Флаг переполнения

// Функция для вычисления следующиего нечётного факториала
static void factorial(int n) {
    if (n == 0 || n == 1) {
        current_fact = 1;
        return;
    }

    if (current_fact > LLONG_MAX / n) {
        overflow_flag = 1;
        return; 
    } else {
        current_fact *= n;
    }

    if (current_fact > LLONG_MAX / (n-1)) {
        overflow_flag = 1;
        return; 
    } else {
        current_fact *= n-1;
    }

    return;
}

// Функция для вычисления следующией нечетной степени x
static long long power(long long int base) {
    if (base==0) {
        overflow_flag = 1;
        return 0;
    }
    if (x_power > LLONG_MAX / ((long long)base * base)) {
        overflow_flag = 1;
        return 0;
    }

    // Такая запись упрощает отладку, поэтому продублировал
    x_power *= base;
    // Нормируем число, чтобы сохранить масштаб
    x_power /= SCALE;
    
    x_power *= base;
    // Нормируем число, чтобы сохранить масштаб
    x_power /= SCALE;

    return 1;
}

// Чтение данных из /proc/tsulab
static ssize_t proc_read(struct file *file, char __user *buf, size_t count, loff_t *ppos) {
    char *output;
    size_t len;
    long long term_scaled;
    long long int x_scaled = PI / 3; // x = Pi/3, в масштабированном виде

    if (*ppos > 0)
        return 0;

    output = kmalloc(256, GFP_KERNEL);
    if (!output)
        return -ENOMEM;

    if (overflow_flag) {
        len = snprintf(output, 256, 
            "Error: 1 Overflow occurred. Calculations stopped.\n"
            "Term: %d, Final sum: %lld.%06lld\n",
            current_term, sum_scaled / SCALE, llabs(sum_scaled % SCALE));
        goto send_to_user;
    }

    // Вычисление текущего члена ряда
    if (current_term % 2 == 0) {
        term_scaled = x_power / current_fact;
    } else {
        term_scaled = (-1) * x_power / current_fact;
    }

    // Если текущий член ряда слишком мал для дальнейших вычислений, останавливаем
    if (llabs(term_scaled) < 1) { // Если меньше 0.000001
        len = snprintf(output, 256, 
           "Calculations stopped. : %d, Final sum: %lld.%06lld (precision limit reached).\n",
           current_term, sum_scaled / SCALE, llabs(sum_scaled % SCALE));
        goto send_to_user;
    }

    if (sum_scaled > LLONG_MAX - llabs(term_scaled)) {
        overflow_flag = 1;
    }

    // Проверка переполнения после вычисления
    if (overflow_flag) {
        len = snprintf(output, 256, 
            "Error: 2 Overflow occurred. Calculations stopped.\n"
            "Term: %d, Final sum: %lld.%06lld\n",
            current_term, sum_scaled / SCALE, llabs(sum_scaled % SCALE));
        goto send_to_user;
    }

    sum_scaled += term_scaled;

    len = snprintf(output, 256, 
        "term_scaled: %lld, Term: %d, Current sum: %lld.%06lld\n",
        term_scaled, current_term, sum_scaled / SCALE, sum_scaled % SCALE);

    current_term++;

    // обновляем значения
    power(x_scaled);
    factorial(2 * current_term + 1);

send_to_user:
    if (copy_to_user(buf, output, len)) {
        kfree(output);
        return -EFAULT;
    }

    *ppos += len;
    kfree(output);
    return len;
}

static const struct proc_ops proc_file_ops = {
    .proc_read = proc_read,
};

static int __init tsulab_init(void) {
    proc_file = proc_create(PROC_NAME, 0444, NULL, &proc_file_ops);
    if (!proc_file) {
        pr_err("Failed to create /proc/%s\n", PROC_NAME);
        return -ENOMEM;
    }

    printk(KERN_INFO "Welcome to the Tomsk State University\n");
    return 0;
}

static void __exit tsulab_exit(void) {
    remove_proc_entry(PROC_NAME, NULL);
    printk(KERN_INFO "Tomsk State University forever!\n");
}

module_init(tsulab_init);
module_exit(tsulab_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("DaniilSelin");
MODULE_DESCRIPTION("Taylor series for sin(Pi/3)");

#include <linux/init.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/timekeeping.h>
#include <linux/time64.h>
#include <linux/mutex.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 41");
MODULE_DESCRIPTION("Prints current time and time since last read via /proc/timer");

#define ENTRY_NAME "timer"
#define PERMS 0444
#define PARENT NULL
#define BUF_LEN 128

static struct proc_dir_entry *timer_entry;
static struct timespec64 last_time;  /* time of the previous read */
static bool has_last;                /* false until the first read happens */
static DEFINE_MUTEX(timer_lock);     /* protects last_time and has_last */

static ssize_t timer_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    struct timespec64 now;
    struct timespec64 elapsed;
    char buf[BUF_LEN];
    int len;

    /* cat keeps calling read until it gets 0; only take a timestamp on the first call */
    if (*ppos > 0)
        return 0;

    ktime_get_real_ts64(&now);

    mutex_lock(&timer_lock);
    len = scnprintf(buf, sizeof(buf), "current time: %lld.%09ld\n",
                    (long long)now.tv_sec, now.tv_nsec);
    if (has_last) {
        elapsed = timespec64_sub(now, last_time);
        len += scnprintf(buf + len, sizeof(buf) - len, "elapsed time: %lld.%09ld\n",
                         (long long)elapsed.tv_sec, elapsed.tv_nsec);
    }
    last_time = now;
    has_last = true;
    mutex_unlock(&timer_lock);

    return simple_read_from_buffer(ubuf, count, ppos, buf, len);
}

static const struct proc_ops timer_fops = {
    .proc_read = timer_read,
};

static int __init timer_init(void)
{
    timer_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &timer_fops);
    if (!timer_entry)
        return -ENOMEM;
    return 0;
}

static void __exit timer_exit(void)
{
    proc_remove(timer_entry);
}

module_init(timer_init);
module_exit(timer_exit);

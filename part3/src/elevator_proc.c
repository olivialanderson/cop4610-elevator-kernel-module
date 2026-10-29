#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "elevator.h"

#define ENTRY_NAME "elevator"
#define PERMS 0444
#define PARENT NULL
#define PROC_BUF_LEN 10000  /* handout: proc output is at most 10,000 characters */

static struct proc_dir_entry *elevator_entry;

static const char *state_name(enum elevator_state state)
{
    switch (state) {
    case OFFLINE: return "OFFLINE";
    case IDLE:    return "IDLE";
    case LOADING: return "LOADING";
    case UP:      return "UP";
    case DOWN:    return "DOWN";
    }
    return "UNKNOWN";
}

static ssize_t elevator_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
    struct pet *p;
    char *buf;
    int len = 0;
    int floor;
    ssize_t ret;

    /* Build the text once per read; the follow-up call from cat just gets EOF */
    if (*ppos > 0)
        return 0;

    /* Heap buffer: 10 KB is too big for the small kernel stack */
    buf = kmalloc(PROC_BUF_LEN, GFP_KERNEL);
    if (!buf)
        return -ENOMEM;

    mutex_lock(&elevator_lock);
    len += scnprintf(buf + len, PROC_BUF_LEN - len, "Elevator state: %s\n",
                     state_name(the_elevator.state));
    len += scnprintf(buf + len, PROC_BUF_LEN - len, "Current floor: %d\n",
                     the_elevator.current_floor);
    len += scnprintf(buf + len, PROC_BUF_LEN - len, "Current load: %d lbs\n",
                     the_elevator.load);
    len += scnprintf(buf + len, PROC_BUF_LEN - len, "Elevator status:");
    list_for_each_entry(p, &the_elevator.riders, list)
        len += scnprintf(buf + len, PROC_BUF_LEN - len, " %c%d",
                         pet_letter(p->type), p->dest_floor);
    len += scnprintf(buf + len, PROC_BUF_LEN - len, "\n\n");

    for (floor = MAX_FLOOR; floor >= MIN_FLOOR; floor--) {
        len += scnprintf(buf + len, PROC_BUF_LEN - len, "[%c] Floor %d: %d",
                         floor == the_elevator.current_floor ? '*' : ' ',
                         floor, the_building.num_waiting[floor - 1]);
        list_for_each_entry(p, &the_building.floors[floor - 1], list)
            len += scnprintf(buf + len, PROC_BUF_LEN - len, " %c%d",
                             pet_letter(p->type), p->dest_floor);
        len += scnprintf(buf + len, PROC_BUF_LEN - len, "\n");
    }

    len += scnprintf(buf + len, PROC_BUF_LEN - len, "\nNumber of pets: %d\n",
                     the_elevator.num_riders);
    len += scnprintf(buf + len, PROC_BUF_LEN - len, "Number of pets waiting: %d\n",
                     the_building.total_waiting);
    len += scnprintf(buf + len, PROC_BUF_LEN - len, "Number of pets serviced: %d\n",
                     the_building.total_serviced);
    mutex_unlock(&elevator_lock);

    /* Copy to user space after unlocking: copy_to_user may sleep */
    ret = simple_read_from_buffer(ubuf, count, ppos, buf, len);
    kfree(buf);
    return ret;
}

static const struct proc_ops elevator_fops = {
    .proc_read = elevator_read,
};

int elevator_proc_create(void)
{
    elevator_entry = proc_create(ENTRY_NAME, PERMS, PARENT, &elevator_fops);
    if (!elevator_entry)
        return -ENOMEM;
    return 0;
}

void elevator_proc_remove(void)
{
    proc_remove(elevator_entry);
}

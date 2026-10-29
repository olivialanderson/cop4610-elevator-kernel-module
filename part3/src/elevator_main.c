#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include "elevator.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 41");
MODULE_DESCRIPTION("Pet elevator scheduler with syscalls and /proc/elevator");

/* Function pointers defined in the kernel (syscalls/syscalls.c); we point them at our handlers */
extern long (*STUB_start_elevator)(void);
extern long (*STUB_issue_request)(int, int, int);
extern long (*STUB_stop_elevator)(void);

struct elevator the_elevator;
struct building the_building;
DEFINE_MUTEX(elevator_lock);

static struct task_struct *elevator_thread;

/* Syscall 548. Returns 1 if already active, 0 on success. */
static long start_elevator(void)
{
    mutex_lock(&elevator_lock);
    if (the_elevator.state != OFFLINE) {
        mutex_unlock(&elevator_lock);
        return 1;
    }
    the_elevator.state = IDLE;
    the_elevator.current_floor = MIN_FLOOR;
    the_elevator.load = 0;
    the_elevator.num_riders = 0;
    the_elevator.stopping = false;
    mutex_unlock(&elevator_lock);
    return 0;
}

/* Syscall 549. Returns 1 for an invalid request, -ENOMEM if allocation fails, 0 on success. */
static long issue_request(int start_floor, int destination_floor, int type)
{
    struct pet *p;

    if (start_floor < MIN_FLOOR || start_floor > MAX_FLOOR ||
        destination_floor < MIN_FLOOR || destination_floor > MAX_FLOOR ||
        start_floor == destination_floor ||
        type < 0 || type >= NUM_PET_TYPES)
        return 1;

    /* Allocate before locking: kmalloc(GFP_KERNEL) may sleep */
    p = pet_create(type, start_floor, destination_floor);
    if (!p)
        return -ENOMEM;

    mutex_lock(&elevator_lock);
    list_add_tail(&p->list, &the_building.floors[start_floor - 1]);   /* FIFO: add at tail */
    the_building.num_waiting[start_floor - 1]++;
    the_building.total_waiting++;
    mutex_unlock(&elevator_lock);
    return 0;
}

/* Syscall 550. Returns 1 if already deactivating, 0 otherwise. */
static long stop_elevator(void)
{
    long ret = 0;

    mutex_lock(&elevator_lock);
    if (the_elevator.stopping)
        ret = 1;
    else if (the_elevator.state != OFFLINE)
        the_elevator.stopping = true;   /* kthread drains riders, then sets OFFLINE */
    mutex_unlock(&elevator_lock);
    return ret;
}

static int __init elevator_init(void)
{
    int i;
    int ret;

    the_elevator.state = OFFLINE;
    the_elevator.current_floor = MIN_FLOOR;
    the_elevator.load = 0;
    the_elevator.num_riders = 0;
    the_elevator.stopping = false;
    INIT_LIST_HEAD(&the_elevator.riders);

    for (i = 0; i < NUM_FLOORS; i++) {
        INIT_LIST_HEAD(&the_building.floors[i]);
        the_building.num_waiting[i] = 0;
    }
    the_building.total_waiting = 0;
    the_building.total_serviced = 0;

    ret = elevator_proc_create();
    if (ret)
        return ret;

    elevator_thread = kthread_run(elevator_thread_fn, NULL, "elevator_thread");
    if (IS_ERR(elevator_thread)) {
        elevator_proc_remove();
        return PTR_ERR(elevator_thread);
    }

    /* Connect the syscalls last, once everything they use is ready */
    STUB_start_elevator = start_elevator;
    STUB_issue_request = issue_request;
    STUB_stop_elevator = stop_elevator;
    return 0;
}

static void __exit elevator_exit(void)
{
    int i;

    /* Disconnect syscalls first so no new requests arrive during cleanup */
    STUB_start_elevator = NULL;
    STUB_issue_request = NULL;
    STUB_stop_elevator = NULL;

    kthread_stop(elevator_thread);   /* waits for the thread loop to return */
    elevator_proc_remove();

    mutex_lock(&elevator_lock);
    free_pet_list(&the_elevator.riders);
    for (i = 0; i < NUM_FLOORS; i++)
        free_pet_list(&the_building.floors[i]);
    mutex_unlock(&elevator_lock);
}

module_init(elevator_init);
module_exit(elevator_exit);

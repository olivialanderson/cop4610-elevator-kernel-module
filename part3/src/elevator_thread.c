#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include "elevator.h"

/* Caller holds elevator_lock. Ignore waiting pets while stopping. */
static bool work_ahead(int direction)
{
    struct pet *p;
    int floor;

    list_for_each_entry(p, &the_elevator.riders, list) {
        if ((direction > 0 && p->dest_floor > the_elevator.current_floor) ||
            (direction < 0 && p->dest_floor < the_elevator.current_floor))
            return true;
    }

    if (!the_elevator.stopping) {
        for (floor = the_elevator.current_floor + direction;
             floor >= MIN_FLOOR && floor <= MAX_FLOOR;
             floor += direction) {
            if (!list_empty(&the_building.floors[floor - MIN_FLOOR]))
                return true;
        }
    }

    return false;
}

int elevator_thread_fn(void *data)
{
    struct pet *p;
    struct pet *tmp;
    struct list_head *queue;
    int direction = 1;
    int floor_index;
    bool serviced;

    while (!kthread_should_stop()) {
        mutex_lock(&elevator_lock);

        if (the_elevator.state == OFFLINE) {
            direction = 1;
            mutex_unlock(&elevator_lock);
            msleep(100);
            continue;
        }

        serviced = false;
        list_for_each_entry_safe(p, tmp, &the_elevator.riders, list) {
            if (p->dest_floor == the_elevator.current_floor) {
                list_del(&p->list);
                the_elevator.load -= p->weight;
                the_elevator.num_riders--;
                the_building.total_serviced++;
                kfree(p);
                serviced = true;
            }
        }

        if (!the_elevator.stopping) {
            floor_index = the_elevator.current_floor - MIN_FLOOR;
            queue = &the_building.floors[floor_index];

            while (!list_empty(queue)) {
                p = list_first_entry(queue, struct pet, list);

                /* Do not skip a pet that cannot fit: preserve FIFO. */
                if (the_elevator.num_riders >= MAX_PETS ||
                    the_elevator.load + p->weight > MAX_WEIGHT)
                    break;

                list_move_tail(&p->list, &the_elevator.riders);
                the_building.num_waiting[floor_index]--;
                the_building.total_waiting--;
                the_elevator.load += p->weight;
                the_elevator.num_riders++;
                serviced = true;
            }
        }

        if (serviced) {
            the_elevator.state = LOADING;
            mutex_unlock(&elevator_lock);
            ssleep(1);
            mutex_lock(&elevator_lock);

            if (kthread_should_stop()) {
                mutex_unlock(&elevator_lock);
                break;
            }
        }

        if (the_elevator.stopping && the_elevator.num_riders == 0) {
            the_elevator.state = OFFLINE;
            the_elevator.stopping = false;

            direction = 1;
            mutex_unlock(&elevator_lock);
            continue;
        }

        /* Keep moving in the same direction until no work remains ahead. */
        if (!work_ahead(direction)) {
            if (work_ahead(-direction)) {
                direction = -direction;
            } else {
                the_elevator.state = IDLE;
                mutex_unlock(&elevator_lock);
                msleep(100);
                continue;
            }
        }

        the_elevator.state = direction > 0 ? UP : DOWN;
        mutex_unlock(&elevator_lock);
        ssleep(2);
        mutex_lock(&elevator_lock);

        if (kthread_should_stop()) {
            mutex_unlock(&elevator_lock);
            break;
        }

        the_elevator.current_floor += direction;
        mutex_unlock(&elevator_lock);
    }

    return 0;
}

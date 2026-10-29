#include <linux/kthread.h>
#include <linux/delay.h>
#include "elevator.h"

/*
 * Elevator control thread (started at module load, stopped at unload).
 *
 * STAGE 1 PLACEHOLDER: only completes a pending stop so start/stop can be tested.
 * STAGE 2 TODO (Gabe + Gannon): each loop, with elevator_lock held:
 *   1. If OFFLINE: do nothing.
 *   2. Unload riders whose dest_floor == current_floor (update load, num_riders,
 *      total_serviced; kfree each pet).
 *   3. If not stopping: load pets from the current floor's queue in FIFO order
 *      until the NEXT pet would exceed MAX_PETS or MAX_WEIGHT (update
 *      num_waiting, total_waiting, load, num_riders).
 *   4. If anything was loaded/unloaded: state = LOADING, then unlock and ssleep(1).
 *   5. Pick a direction (UP/DOWN one floor, never wrap 5->1), or IDLE if no work.
 *      When moving: unlock, ssleep(2), then lock and update current_floor.
 *   6. If stopping and no riders: state = OFFLINE, stopping = false.
 *   Never call ssleep() while holding elevator_lock.
 */
int elevator_thread_fn(void *data)
{
    while (!kthread_should_stop()) {
        mutex_lock(&elevator_lock);
        if (the_elevator.stopping && the_elevator.num_riders == 0) {
            the_elevator.state = OFFLINE;
            the_elevator.stopping = false;
        }
        mutex_unlock(&elevator_lock);
        msleep(100);
    }
    return 0;
}

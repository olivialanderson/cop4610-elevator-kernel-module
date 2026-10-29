#ifndef ELEVATOR_H
#define ELEVATOR_H

#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/types.h>

#define MIN_FLOOR 1
#define MAX_FLOOR 5
#define NUM_FLOORS (MAX_FLOOR - MIN_FLOOR + 1)
#define MAX_PETS 5          /* elevator capacity in pets */
#define MAX_WEIGHT 50       /* elevator capacity in lbs */
#define NUM_PET_TYPES 4

enum pet_type { CHIHUAHUA = 0, PUG = 1, PUGHUAHUA = 2, DACHSHUND = 3 };
enum elevator_state { OFFLINE, IDLE, LOADING, UP, DOWN };

/* One pet; lives on exactly one list (a floor queue or the elevator) */
struct pet {
    int type;
    int weight;
    int start_floor;
    int dest_floor;
    struct list_head list;
};

struct elevator {
    enum elevator_state state;
    int current_floor;
    int load;               /* total weight of riders in lbs */
    int num_riders;
    bool stopping;          /* stop requested: no loading, drain riders, then OFFLINE */
    struct list_head riders;
};

struct building {
    struct list_head floors[NUM_FLOORS];   /* FIFO queue per floor; index = floor - 1 */
    int num_waiting[NUM_FLOORS];
    int total_waiting;
    int total_serviced;
};

/*
 * Shared state. elevator_lock must be held for ANY read or write of
 * the_elevator or the_building. Never sleep (ssleep/msleep) while holding it.
 */
extern struct elevator the_elevator;
extern struct building the_building;
extern struct mutex elevator_lock;

/* pets.c */
int pet_weight(int type);
char pet_letter(int type);
struct pet *pet_create(int type, int start_floor, int dest_floor);
void free_pet_list(struct list_head *head);

/* elevator_proc.c */
int elevator_proc_create(void);
void elevator_proc_remove(void);

/* elevator_thread.c */
int elevator_thread_fn(void *data);

#endif /* ELEVATOR_H */

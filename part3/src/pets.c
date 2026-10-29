#include <linux/slab.h>
#include "elevator.h"

/* Indexed by enum pet_type */
static const int pet_weights[NUM_PET_TYPES] = { 3, 14, 10, 16 };
static const char pet_letters[NUM_PET_TYPES] = { 'C', 'P', 'H', 'D' };

int pet_weight(int type)
{
    return pet_weights[type];
}

char pet_letter(int type)
{
    return pet_letters[type];
}

/* Allocates a pet; caller must validate arguments first. Returns NULL on failure. */
struct pet *pet_create(int type, int start_floor, int dest_floor)
{
    struct pet *p = kmalloc(sizeof(*p), GFP_KERNEL);

    if (!p)
        return NULL;
    p->type = type;
    p->weight = pet_weights[type];
    p->start_floor = start_floor;
    p->dest_floor = dest_floor;
    INIT_LIST_HEAD(&p->list);
    return p;
}

/* Removes and frees every pet on a list. Caller holds elevator_lock. */
void free_pet_list(struct list_head *head)
{
    struct pet *p;
    struct pet *tmp;

    list_for_each_entry_safe(p, tmp, head, list) {
        list_del(&p->list);
        kfree(p);
    }
}

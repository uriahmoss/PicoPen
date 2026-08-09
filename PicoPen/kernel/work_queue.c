#include "picopen/work_queue.h"

#include <stddef.h>

void picopen_work_queue_init(picopen_work_queue_t *queue) {
    if (queue != NULL) {
        *queue = (picopen_work_queue_t){0};
    }
}

bool picopen_work_schedule(picopen_work_queue_t *queue, uint64_t deadline_ms,
                           picopen_work_callback_t callback, void *context) {
    if ((queue == NULL) || (callback == NULL)) {
        return false;
    }
    for (size_t index = 0u; index < PICOPEN_WORK_QUEUE_CAPACITY; ++index) {
        if (queue->items[index].active) {
            continue;
        }
        queue->items[index] = (picopen_work_item_t){
            .callback = callback,
            .context = context,
            .deadline_ms = deadline_ms,
            .active = true,
        };
        return true;
    }
    return false;
}

size_t picopen_work_run_due(picopen_work_queue_t *queue, uint64_t now_ms,
                            size_t budget) {
    if ((queue == NULL) || (budget == 0u)) {
        return 0u;
    }
    size_t completed = 0u;
    for (size_t index = 0u;
         (index < PICOPEN_WORK_QUEUE_CAPACITY) && (completed < budget); ++index) {
        picopen_work_item_t item = queue->items[index];
        if (!item.active || (item.deadline_ms > now_ms)) {
            continue;
        }
        queue->items[index].active = false;
        item.callback(item.context);
        ++completed;
    }
    return completed;
}

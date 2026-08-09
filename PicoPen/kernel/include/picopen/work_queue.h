#ifndef PICOPEN_WORK_QUEUE_H
#define PICOPEN_WORK_QUEUE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PICOPEN_WORK_QUEUE_CAPACITY 8u

typedef void (*picopen_work_callback_t)(void *context);

typedef struct picopen_work_item {
    picopen_work_callback_t callback;
    void *context;
    uint64_t deadline_ms;
    bool active;
} picopen_work_item_t;

typedef struct picopen_work_queue {
    picopen_work_item_t items[PICOPEN_WORK_QUEUE_CAPACITY];
} picopen_work_queue_t;

void picopen_work_queue_init(picopen_work_queue_t *queue);
bool picopen_work_schedule(picopen_work_queue_t *queue, uint64_t deadline_ms,
                           picopen_work_callback_t callback, void *context);
size_t picopen_work_run_due(picopen_work_queue_t *queue, uint64_t now_ms,
                            size_t budget);

#endif

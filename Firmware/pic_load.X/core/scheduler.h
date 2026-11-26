/**
 * @file    scheduler.h
 * @brief   Cooperative, time-triggered task scheduler.
 *
 * Each task is a plain function that runs to completion and returns: it
 * must never wait in a loop or call long delays. The scheduler is called
 * over and over from main() and runs every task whose period is due.
 *
 * Timing
 *   - Fixed rate: the next run time advances by exactly one period, so a
 *     task does not drift. If a task is late (another task ran long), it
 *     runs again as soon as possible until it has caught up.
 *   - Resolution 1 ms (core/tick.h). Jitter = the longest run time of any
 *     other task.
 *   - Work that must be on time to the millisecond (input sampling, safety)
 *     belongs in the interrupt, not in a task.
 *
 * Example
 * @code
 *     static task_t tasks[] = {
 *         { blink_task, 500, 0 },
 *         { ui_task,     20, 0 },
 *     };
 *     sched_init(tasks, 2);
 *     for (;;) sched_run(tasks, 2);
 * @endcode
 */
#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <stdint.h>

typedef void (*task_fn_t)(void);

/** One entry of the task table. */
typedef struct
{
    task_fn_t fn;           /**< function to run                  */
    uint16_t  period_ms;    /**< run every period_ms milliseconds  */
    uint16_t  next_ms;      /**< next due time (set by sched_init) */
} task_t;

/** Make every task due now. Call once, after tick_init(). */
void sched_init(task_t *tasks, uint8_t count);

/** Run each task that is due, once. Call continuously from main(). */
void sched_run(task_t *tasks, uint8_t count);

#endif /* SCHEDULER_H */

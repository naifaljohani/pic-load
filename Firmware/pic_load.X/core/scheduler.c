/**
 * @file    scheduler.c
 * @brief   Cooperative, time-triggered task scheduler.
 */
#include "scheduler.h"
#include "tick.h"

void sched_init(task_t *tasks, uint8_t count)
{
    uint16_t now = tick_ms();
    for (uint8_t i = 0; i < count; i++)
    {
        tasks[i].next_ms = now;
    }
}

void sched_run(task_t *tasks, uint8_t count)
{
    for (uint8_t i = 0; i < count; i++)
    {
        task_t *t = &tasks[i];
        // due when now >= next; the signed difference handles the 16-bit wrap
        if ((int16_t)(tick_ms() - t->next_ms) >= 0)
        {
            t->next_ms += t->period_ms;     // fixed rate, no drift
            t->fn();
        }
    }
}

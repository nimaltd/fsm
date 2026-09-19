/**
 * @file        fsm.c
 * @brief       Finite state machine and task queue for STM32.
 * @version     2.0.0
 *
 * @author      Nima Askari (NimaLTD)
 * @email       nima.askari@gmail.com
 * @github      https://www.github.com/nimaltd
 * @linkedin    https://www.linkedin.com/in/nimaltd
 * @youtube     https://www.youtube.com/@nimaltd
 * @instagram   https://instagram.com/github.nimaltd
 *
 * @copyright   (c) 2026 Nima Askari (NimaLTD)
 *              SPDX-License-Identifier: Apache-2.0
 *              See LICENSE.md in the project root for the full license text.
 */

/*
 * ****************************************************************************************************
 * Includes
 * ****************************************************************************************************
*/

#include "fsm.h"

#include <stddef.h>

#include "main.h"

/*
 * ****************************************************************************************************
 * Types
 * ****************************************************************************************************
*/

/*****************************************************************************************************/
/**
 * @brief Task queue shared between interrupt context and the main loop.
 */
typedef struct
{
    __IO fsm_fn_t fn[FSM_MAX_TASKS]; /**< Circular buffer of queued tasks. */
    __IO uint32_t head;              /**< Slot the producer writes next.   */
    __IO uint32_t tail;              /**< Slot the consumer reads next.    */

} fsm_queue_t;

/*
 * ****************************************************************************************************
 * Global variables
 * ****************************************************************************************************
*/

/* Static storage is zero initialized by the language, which is exactly the
   empty queue, so spelling out an initializer here would add nothing. */
static fsm_queue_t fsm_queue;

/*
 * ****************************************************************************************************
 * Private function prototypes
 * ****************************************************************************************************
*/

/*****************************************************************************************************/
/**
 * @brief Run one queued task if the queue is not empty.
 */
static void fsm_queue_run(void);

/*
 * ****************************************************************************************************
 * Public function implementations
 * ****************************************************************************************************
*/

/*****************************************************************************************************/
/**
 * @brief Initialize a handle and set the state it starts from.
 *
 * The machine runs first_fn on the next call to fsm_loop(), with no delay.
 *
 * @param[out] handle    Handle to initialize. Must not be NULL.
 * @param[in]  first_fn  State function to start from. Must not be NULL.
 */
void fsm_init(fsm_t *handle, fsm_fn_t first_fn)
{
    assert_param(handle != NULL);
    assert_param(first_fn != NULL);

    if ((handle != NULL) && (first_fn != NULL))
    {
        handle->next_fn  = first_fn;
        handle->delay_ms = 0U;
        handle->time     = HAL_GetTick();
    }
}

/*****************************************************************************************************/
/**
 * @brief Run the queued tasks and the current state.
 *
 * Call this as often as possible from the main loop. Queued tasks are served
 * before the state function, so work handed over by an interrupt is not held
 * up by a state that is still waiting out its delay.
 *
 * @param[in,out] handle  Handle to run. Must not be NULL.
 */
void fsm_loop(fsm_t *handle)
{
    assert_param(handle != NULL);

    if (handle != NULL)
    {
        fsm_queue_run();

        if (handle->next_fn != NULL)
        {
            if (handle->delay_ms == 0U)
            {
                handle->time = HAL_GetTick();
                handle->next_fn();
            }
            else if ((HAL_GetTick() - handle->time) >= handle->delay_ms)
            {
                /* Clear the delay before the state runs, so the state itself is
                   free to ask for a new one. */
                handle->delay_ms = 0U;
                handle->time     = HAL_GetTick();
                handle->next_fn();
            }
            else
            {
                /* Still waiting out the delay, nothing to do. */
            }
        }
    }
}

/*****************************************************************************************************/
/**
 * @brief Choose the next state, optionally after a delay.
 *
 * The delay is measured from this call, not from when the current state
 * returns. A delay of zero runs the next state on the following fsm_loop().
 *
 * @param[in,out] handle    Handle to update. Must not be NULL.
 * @param[in]     next_fn   State function to run next. Must not be NULL.
 * @param[in]     delay_ms  Milliseconds to wait before running next_fn.
 */
void fsm_next(fsm_t *handle, fsm_fn_t next_fn, uint32_t delay_ms)
{
    assert_param(handle != NULL);
    assert_param(next_fn != NULL);

    if ((handle != NULL) && (next_fn != NULL))
    {
        handle->delay_ms = delay_ms;
        handle->time     = HAL_GetTick();
        handle->next_fn  = next_fn;
    }
}

/*****************************************************************************************************/
/**
 * @brief Get how long the machine has been in the current state.
 *
 * The count restarts every time a state function is entered, so a state can
 * use it to measure its own runtime.
 *
 * @param[in] handle  Handle to read. Must not be NULL.
 * @return Milliseconds since the current state was entered, or 0 if handle is NULL.
 */
uint32_t fsm_time(const fsm_t *handle)
{
    uint32_t elapsed = 0U;

    assert_param(handle != NULL);

    if (handle != NULL)
    {
        elapsed = HAL_GetTick() - handle->time;
    }

    return elapsed;
}

/*****************************************************************************************************/
/**
 * @brief Add a task to the queue.
 *
 * Safe to call from an interrupt. The task runs later, from fsm_loop(), which
 * is what keeps the interrupt handler short.
 *
 * @param[in] task_fn  Task to queue. Must not be NULL.
 * @return FSM_ERR_NONE if the task was queued, FSM_ERR_FULL if the queue is
 *         full or task_fn is NULL.
 */
fsm_err_t fsm_task_add(fsm_fn_t task_fn)
{
    fsm_err_t err = FSM_ERR_FULL;

    assert_param(task_fn != NULL);

    if (task_fn != NULL)
    {
        uint32_t head      = fsm_queue.head;
        uint32_t next_head = (head + 1U) % FSM_MAX_TASKS;

        /* Leaving one slot free is what lets a full queue be told apart from
           an empty one, since both would otherwise have head == tail. */
        if (next_head != fsm_queue.tail)
        {
            fsm_queue.fn[head] = task_fn;

            /* The slot has to be visible before head publishes it, or the main
               loop can read a stale pointer out of it. */
            __DMB();

            fsm_queue.head = next_head;
            err            = FSM_ERR_NONE;
        }
    }

    return err;
}

/*
 * ****************************************************************************************************
 * Private function implementations
 * ****************************************************************************************************
*/

/*****************************************************************************************************/
/**
 * @brief Run one queued task if the queue is not empty.
 *
 * Only one task runs per call. That keeps a burst of queued work from starving
 * the state machine, since fsm_loop() gets to run a state in between.
 */
static void fsm_queue_run(void)
{
    if (fsm_queue.tail != fsm_queue.head)
    {
        uint32_t tail    = fsm_queue.tail;
        fsm_fn_t task_fn = fsm_queue.fn[tail];

        /* Release the slot before running the task, so the task is free to
           queue another one without hitting a queue that is falsely full. */
        fsm_queue.tail = (tail + 1U) % FSM_MAX_TASKS;

        __DMB();

        if (task_fn != NULL)
        {
            task_fn();
        }
    }
}

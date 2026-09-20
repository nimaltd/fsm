/**
 * @file        fsm.h
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

#ifndef FSM_H
#define FSM_H

/*
 * ****************************************************************************************************
 * Includes
 * ****************************************************************************************************
*/

#include <stdbool.h>
#include <stdint.h>

#include "fsm_config.h"

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * ****************************************************************************************************
 * Types
 * ****************************************************************************************************
*/

/*****************************************************************************************************/
/**
 * @brief A state function or a queued task. Takes nothing, returns nothing.
 */
typedef void (*fsm_fn_t)(void);

/*****************************************************************************************************/
/**
 * @brief Error values returned by the task queue.
 */
typedef enum
{
    FSM_ERR_NONE = 0, /**< The task was queued.    */
    FSM_ERR_FULL = 1, /**< The task queue is full. */

} fsm_err_t;

/*****************************************************************************************************/
/**
 * @brief State machine handle. Declare one per state machine.
 */
typedef struct
{
    fsm_fn_t next_fn;  /**< State function to run next.                    */
    uint32_t time;     /**< Tick value when the current state was entered. */
    uint32_t delay_ms; /**< Delay to wait before the next state runs.      */
    uint8_t  entering; /**< Set until the next state has run once.         */

} fsm_t;

/*
 * ****************************************************************************************************
 * Public function prototypes
 * ****************************************************************************************************
*/

/*****************************************************************************************************/
/**
 * @brief Initialize a handle and set the state it starts from.
 */
void fsm_init(fsm_t *handle, fsm_fn_t first_fn);

/*****************************************************************************************************/
/**
 * @brief Run the queued tasks and the current state. Call this from the main loop.
 */
void fsm_loop(fsm_t *handle);

/*****************************************************************************************************/
/**
 * @brief Choose the next state, optionally after a delay in milliseconds.
 */
void fsm_next(fsm_t *handle, fsm_fn_t next_fn, uint32_t delay_ms);

/*****************************************************************************************************/
/**
 * @brief Get how long the machine has been in the current state, in milliseconds.
 */
uint32_t fsm_time(const fsm_t *handle);

/*****************************************************************************************************/
/**
 * @brief Stop the machine. No state runs until fsm_next() or fsm_init() is called.
 *
 * Useful for a terminal state, which would otherwise have to keep scheduling
 * itself to stay put.
 *
 * @param[in,out] handle  Handle to stop. Must not be NULL.
 */
void fsm_stop(fsm_t *handle);

/*****************************************************************************************************/
/**
 * @brief Whether the machine has a state to run.
 *
 * @param[in] handle  Handle to read. Must not be NULL.
 * @return true while a state is scheduled, false after fsm_stop().
 */
bool fsm_running(const fsm_t *handle);

/*****************************************************************************************************/
/**
 * @brief The most tasks that have ever been queued at once.
 *
 * There to size FSM_MAX_TASKS by measurement rather than by guessing. A full
 * queue is reported by fsm_task_add(), but that call is usually made from an
 * interrupt where nobody checks the result, so this is the only practical way
 * to find out the queue was ever close to full.
 *
 * @return Peak number of queued tasks since reset.
 */
uint32_t fsm_task_peak(void);

/*****************************************************************************************************/
/**
 * @brief Drop every queued task.
 *
 * Call it from the main loop, not from an interrupt.
 */
void fsm_task_flush(void);

/*****************************************************************************************************/
/**
 * @brief Add a task to the queue. Safe to call from an interrupt.
 */
fsm_err_t fsm_task_add(fsm_fn_t task_fn);

#ifdef __cplusplus
}
#endif

#endif /* FSM_H */

/**
 * @file        fsm.h
 * @brief       Finite state machine and task queue for STM32.
 * @version     2.0.0
 *
 * @author      Nima Askari - NimaLTD
 * @github      https://www.github.com/nimaltd
 * @linkedin    https://www.linkedin.com/in/nimaltd
 * @youtube     https://www.youtube.com/@nimaltd
 * @instagram   https://instagram.com/github.nimaltd
 *
 * @copyright   (c) 2026 Nima Askari - NimaLTD
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
 * @brief Add a task to the queue. Safe to call from an interrupt.
 */
fsm_err_t fsm_task_add(fsm_fn_t task_fn);

#ifdef __cplusplus
}
#endif

#endif /* FSM_H */

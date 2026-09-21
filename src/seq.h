/**
 * @file        seq.h
 * @brief       Non blocking state sequencer and interrupt task queue for STM32.
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

#ifndef SEQ_H
#define SEQ_H

/*
 * ****************************************************************************************************
 * Includes
 * ****************************************************************************************************
*/

#include <stdbool.h>
#include <stdint.h>

#include "seq_config.h"

/* One slot is always kept free so a full queue can be told apart from an empty
   one, so the queue needs at least two slots to work at all. Checked here
   rather than in seq_config.h, because that file is the user's copy and is
   never replaced, so a check living there would never reach anyone who
   installed before it was added. */
#if SEQ_MAX_TASKS < 2U
#error "SEQ_MAX_TASKS must be at least 2"
#endif

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
 * @brief State machine handle, declared ahead so a state function can take one.
 */
typedef struct seq_s seq_t;

/*****************************************************************************************************/
/**
 * @brief A state function. Gets the handle it belongs to.
 */
typedef void (*seq_state_fn_t)(seq_t *handle);

/*****************************************************************************************************/
/**
 * @brief A queued task. Gets whatever was handed to seq_task_add().
 */
typedef void (*seq_task_fn_t)(void *arg);

/*****************************************************************************************************/
/**
 * @brief Error values returned by the task queue.
 */
typedef enum
{
    SEQ_ERR_NONE    = 0, /**< The task was queued.        */
    SEQ_ERR_FULL    = 1, /**< The task queue is full.     */
    SEQ_ERR_INVALID = 2, /**< The task pointer was NULL.  */

} seq_err_t;

/*****************************************************************************************************/
/**
 * @brief State machine handle. Declare one per state machine.
 */
struct seq_s
{
    seq_state_fn_t next_fn;  /**< State function to run next.                    */
    void           *user;    /**< Yours. Given to seq_init(), never read here.   */
    uint32_t       time;     /**< Tick value when the current state was entered. */
    uint32_t       delay_ms; /**< Delay to wait before the next state runs.      */
    uint8_t        entering; /**< Set until the next state has run once.         */
};

/*
 * ****************************************************************************************************
 * Public function prototypes
 * ****************************************************************************************************
*/

/*****************************************************************************************************/
/**
 * @brief Initialize a handle, set the state it starts from, and keep user for it.
 */
void seq_init(seq_t *handle, seq_state_fn_t first_fn, void *user);

/*****************************************************************************************************/
/**
 * @brief Run the queued tasks and the current state. Call this from the main loop.
 */
void seq_loop(seq_t *handle);

/*****************************************************************************************************/
/**
 * @brief Choose the next state, optionally after a delay in milliseconds.
 */
void seq_next(seq_t *handle, seq_state_fn_t next_fn, uint32_t delay_ms);

/*****************************************************************************************************/
/**
 * @brief Get how long the machine has been in the current state, in milliseconds.
 */
uint32_t seq_time(const seq_t *handle);

/*****************************************************************************************************/
/**
 * @brief Stop the machine. No state runs until seq_next() or seq_init() is called.
 */
void seq_stop(seq_t *handle);

/*****************************************************************************************************/
/**
 * @brief Whether the machine has a state to run. False after seq_stop().
 */
bool seq_running(const seq_t *handle);

/*****************************************************************************************************/
/**
 * @brief The most tasks that have ever been queued at once, for sizing SEQ_MAX_TASKS.
 */
uint32_t seq_task_peak(void);

/*****************************************************************************************************/
/**
 * @brief Drop every queued task. Call it from the main loop, not an interrupt.
 */
void seq_task_flush(void);

/*****************************************************************************************************/
/**
 * @brief Queue a task with its argument. Safe to call from an interrupt.
 */
seq_err_t seq_task_add(seq_task_fn_t task_fn, void *arg);

#ifdef __cplusplus
}
#endif

#endif /* SEQ_H */

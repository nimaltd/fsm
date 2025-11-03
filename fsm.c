
/*
 * @file        fsm.c
 * @brief       Finite State Machine + task manager Library
 * @author      Nima Askari
 * @version     1.0.0
 * @license     See the LICENSE file in the root folder.
 *
 * @note        All my libraries are dual-licensed.
 *              Please review the licensing terms before using them.
 *              For any inquiries, feel free to contact me.
 *
 * @github      https://www.github.com/nimaltd
 * @linkedin    https://www.linkedin.com/in/nimaltd
 * @youtube     https://www.youtube.com/@nimaltd
 * @instagram   https://instagram.com/github.nimaltd
 *
 * Copyright (C) 2025 Nima Askari - NimaLTD. All rights reserved.
 */

/*************************************************************************************************/
/** Includes **/
/*************************************************************************************************/

#include "fsm.h"

/*************************************************************************************************/
/** Global variables **/
/*************************************************************************************************/

fsm_task_t fsm_task = { .head = 0, .tail = 0 };

/*************************************************************************************************/
/** Private function prototype **/
/*************************************************************************************************/

static void fsm_task_loop(void);

/*************************************************************************************************/
/** Function Implementations **/
/*************************************************************************************************/

/*************************************************************************************************/
/**
 * @brief Initializes the finite state machine (FSM) handle.
 * @param[in,out] handle: Pointer to the FSM handle to initialize.
 * @param[in] first_fn: Pointer to the first of FSM state functions.
 */
void fsm_init(fsm_t *handle, const void (*first_fn)(void))
{
  assert_param(handle != NULL);
  assert_param(first_fn != NULL);

  /* Set the initial FSM state function */
  handle->next_fn = first_fn;

  /* Reset all values */
  handle->delay_ms = 0;

  handle->time = HAL_GetTick();
}

/*************************************************************************************************/
/**
 * @brief FSM main loop handler.
 *        Executes scheduled tasks or advances the FSM state when delays expire.
 * @param[in,out] handle: Pointer to the FSM handle.
 */
void fsm_loop(fsm_t *handle)
{
  assert_param(handle != NULL);
  assert_param(handle->next_fn != NULL);

  /* Check task queue */
  fsm_task_loop();

  /* Execute current state function if no delay is active */
  if (handle->delay_ms == 0)
  {
    /* Save current tick time */
    handle->time = HAL_GetTick();

    /* Run state function */
    handle->next_fn();
  }
  else
  {
    /* Check if delay expired */
    if (HAL_GetTick() - handle->time >= handle->delay_ms)
    {
      handle->delay_ms = 0;

      /* Update reference time */
      handle->time = HAL_GetTick();

      /* Run state function */
      handle->next_fn();
    }
  }
}

/*************************************************************************************************/
/**
 * @brief Sets the next state function for the FSM with optional delay.
 * @param[in,out] handle: Pointer to the FSM handle.
 * @param[in] next_fn: Next state function to execute.
 * @param[in] delay_ms: None-blocking delay in milliseconds before execution.
 */
void fsm_next(fsm_t *handle, const void (*next_fn)(void), uint32_t delay_ms)
{
  assert_param(handle != NULL);
  assert_param(next_fn != NULL);

  /* Set delay before next state execution */
  handle->delay_ms = delay_ms;

  /* Record the current time for delay tracking */
  handle->time = HAL_GetTick();

  /* Assign the next state function */
  handle->next_fn = next_fn;
}

/*************************************************************************************************/
/**
 * @brief Returns elapsed time in the current FSM state.
 * @param[in] handle: Pointer to the FSM handle.
 * @return uint32_t: Elapsed time in milliseconds since entering the current state.
 */
uint32_t fsm_time(fsm_t *handle)
{
  assert_param(handle != NULL);

  /* Calculate and return elapsed time */
  return (HAL_GetTick() - handle->time);
}

/*************************************************************************************************/
/**
 * @brief Adds a new task function to the FSM task queue (lock-free, ISR-safe).
 * @param[in] new_task_fn: Pointer to the task function to add to the queue.
 * @return fsm_err_t: Returns FSM_ERR_NONE if successful, FSM_ERR_FULL if queue is full.
 */
fsm_err_t fsm_task_add(const void (*new_task_fn)(void))
{
  assert_param(new_task_fn != NULL);

  /* Read the current head index (write position) */
  uint32_t head = fsm_task.head;

  /* Compute the next head index with wrap-around */
  uint32_t next_head = (head + 1U) % FSM_MAX_TASKS;

  /* Check if the queue is full */
  if (next_head == fsm_task.tail)
  {
    return FSM_ERR_FULL;
  }

  /* Store the new task function in the queue at the current head position */
  fsm_task.fn[head] = new_task_fn;

  /* Ensures that the write to fn[head] completes and becomes visible
     *before* the head index is updated */
  __DMB();

  /* Finally, update the head index to publish the new task to the queue */
  fsm_task.head = next_head;

  return FSM_ERR_NONE;
}

/*************************************************************************************************/
/** Private Function Implementations **/
/*************************************************************************************************/

/*************************************************************************************************/
/**
 * @brief Runs all queued tasks.
 */
static void fsm_task_loop(void)
{
  /* Check if the task queue is not empty */
  if (fsm_task.tail != fsm_task.head)
  {
    /* Read the current tail index (read position) */
    uint32_t tail = fsm_task.tail;

    /* Fetch the task function pointer from the queue */
    void (*task_fn)(void) = fsm_task.fn[tail];

    /* Advance the tail index before executing the task */
    fsm_task.tail = (tail + 1U) % FSM_MAX_TASKS;

    /* Ensures that the tail update and memory writes are visible to all
       contexts before continuing */
    __DMB();

    /* Execute the retrieved task function if it is valid */
    if (task_fn != NULL)
    {
      task_fn();
    }
  }
}

/*************************************************************************************************/
/** End of File **/
/*************************************************************************************************/


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
  handle->task_head = 0;
  handle->task_tail = 0;
  handle->task_cnt  = 0;
  handle->delay_ms = 0;
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

  /* If there are pending tasks in the queue, execute the oldest one */
  if (handle->task_cnt > 0)
  {
    /* Run task and Move tail pointer*/
    handle->task_fn[handle->task_tail]();
    handle->task_tail = (handle->task_tail + 1) % FSM_MAX_TASKS;
    handle->task_cnt--;
    return;
  }

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
 * @brief Adds a new task function to the FSM task queue.
 * @param[in,out] handle: Pointer to the FSM handle.
 * @param[in] new_task_fn: Pointer to the task function to add to the queue.
 * @return fsm_err_t: Returns FSM_ERR_NONE if successful, FSM_ERR_FULL if queue is full.
 */
fsm_err_t fsm_task_add(fsm_t *handle, const void (*new_task_fn)(void))
{
  assert_param(handle != NULL);
  assert_param(new_task_fn != NULL);

  /* Check if there is space in the task queue */
  if (handle->task_cnt < FSM_MAX_TASKS)
  {
    /* Add task to queue */
    handle->task_fn[handle->task_head] = new_task_fn;
    handle->task_head = (handle->task_head + 1) % FSM_MAX_TASKS;
    handle->task_cnt++;
    return FSM_ERR_NONE;
  }
  else
  {
    return FSM_ERR_FULL;
  }
}

/*************************************************************************************************/
/** End of File **/
/*************************************************************************************************/

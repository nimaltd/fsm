
/*
 * @file        FSM
 * @author      Nima Askari
 * @version     1.0.0
 * @license     See the LICENSE file in the root folder.
 *
 * @github      https://www.github.com/nimaltd
 * @linkedin    https://www.linkedin.com/in/nimaltd
 * @youtube     https://www.youtube.com/@nimaltd
 * @instagram   https://instagram.com/github.nimaltd
 *
 * Copyright (C) 2025 Nima Askari - NimaLTD. All rights reserved.
 *
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
 *
 * @param[in,out] handle Pointer to the FSM handle to initialize.
 * @param[in] first_fn   Pointer to the first of FSM state functions.
 */
void fsm_init(fsm_t *handle, const void (*first_fn)(void))
{
  assert_param(handle != NULL);
  assert_param(first != NULL);
  handle->next_fn = first_fn;
  handle->delay_ms = 0;
}

/*************************************************************************************************/
/**
 * @brief Runs the FSM state machine loop.
 *
 * Executes the next state function if the delay has elapsed.
 *
 * @param[in,out] handle Pointer to the FSM handle.
 */
void fsm_loop(fsm_t *handle)
{
  assert_param(handle != NULL);
  assert_param(handle->next != NULL);
  if (handle->delay_ms == 0)
  {
    handle->time = HAL_GetTick();
    handle->next_fn();
  }
  else
  {
    if (HAL_GetTick() - handle->time >= handle->delay_ms)
    {
      handle->delay_ms = 0;
      handle->time = HAL_GetTick();
      handle->next_fn();
    }
  }
}

/*************************************************************************************************/
/**
 * @brief Sets the next state function for the FSM.
 *
 * @param[in,out] handle Pointer to the FSM handle.
 * @param[in] next_fn   Next state function to execute.
 * @param[in] delay_ms  None blocking delay in milliseconds.
 */
void fsm_next(fsm_t *handle, const void (*next_fn)(void), uint32_t delay_ms)
{
  assert_param(handle != NULL);
  assert_param(next != NULL);
  handle->delay_ms = delay_ms;
  handle->time = HAL_GetTick();
  handle->next_fn = next_fn;
}

/*************************************************************************************************/
/**
 * @brief Returns elapsed time in the current FSM state.
 *
 * @param[in] handle Pointer to the FSM handle.
 * @return uint32_t Elapsed time in milliseconds since entering the current state.
 */
uint32_t fsm_time(fsm_t *handle)
{
  assert_param(handle != NULL);
  return (HAL_GetTick() - handle->time);
}

/*************************************************************************************************/
/** End of File **/
/*************************************************************************************************/

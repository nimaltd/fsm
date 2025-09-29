
/*
 * @file        fsm.h
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

#ifndef _FSM_H_
#define _FSM_H_

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************************************/
/** Includes **/
/*************************************************************************************************/

#include "main.h"
#include "fsm_config.h"

/*************************************************************************************************/
/** Typedef/Struct/Enum **/
/*************************************************************************************************/

/*************************************************************************************************/
/* Error values returned */
typedef enum
{
  FSM_ERR_NONE        = 0,  /* No error */
  FSM_ERR_FULL,             /* Task queue is full */

} fsm_err_t;

/*************************************************************************************************/
/* Internal state machine structure */
typedef struct
{
  void                (*next_fn)(void);                 /* Pointer to the next state function */
  void                (*task_fn[FSM_MAX_TASKS])(void);  /* Circular buffer of task functions */
  uint32_t            time;                             /* Internal time counter (ms) */
  uint32_t            delay_ms;                         /* Delay before switching to next state */
  uint32_t            task_cnt;                         /* Number of tasks in queue */
  uint32_t            task_head;                        /* Head index of the task queue */
  uint32_t            task_tail;                        /* Tail index of the task queue */

} fsm_t;

/*************************************************************************************************/
/** API Functions **/
/*************************************************************************************************/

/* Initialize FSM instance */
void      fsm_init(fsm_t *handle, const void (*first_fn)(void));

/* Main loop handler. Should be called periodically */
void      fsm_loop(fsm_t *handle);

/* Change FSM to the next state after a delay */
void      fsm_next(fsm_t *handle, const void (*next_fn)(void), uint32_t delay_ms);

/* Get internal FSM time */
uint32_t  fsm_time(fsm_t *handle);

/* Add a task function to FSM task queue */
fsm_err_t fsm_task_add(fsm_t *handle, const void (*new_task_fn)(void));

/*************************************************************************************************/
/** End of File **/
/*************************************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* _FSM_H_ */

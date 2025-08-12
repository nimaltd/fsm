
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

/*
 *  FSM (Finite State Machine) Library Help
 *
 *  Overview:
 *  This library provides a lightweight framework to manage state-based logic
 *  in embedded applications. Each state is represented by a function, and
 *  the FSM structure keeps track of the current state, next state, and
 *  optional timed transitions.
 *
 *  Usage:
 *  1. Define your state functions:
 *      void state_idle(void)
 *      {
 *          // Do idle actions here
 *          // Transition immediately to run state
 *          fsm_next(&myfsm, state_run, 0);
 *      }
 *
 *      void state_run(void)
 *      {
 *          // Do running actions here
 *          // After 1000 ms, go back to idle state
 *          fsm_next(&myfsm, state_idle, 1000);
 *      }
 *
 *  2. Create and initialize an FSM instance:
 *      fsm_t myfsm;
 *      fsm_init(&myfsm, state_idle);
 *
 *  3. In your main loop, call:
 *      while (1)
 *      {
 *          fsm_loop(&myfsm);
 *      }
 *
 *  Notes:
 *  - All state functions must match the signature: void func(void).
 *  - Delays are non-blocking; they rely on a time source (e.g., HAL_GetTick()).
 *  - You can change state either from main code or inside a state function.
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

/*************************************************************************************************/
/** Strucs and Enums **/
/*************************************************************************************************/

typedef struct
{
  void                (*next_fn)(void);
  uint32_t            time;
  uint32_t            delay_ms;

} fsm_t;

/*************************************************************************************************/
/** Function Declarations **/
/*************************************************************************************************/

void      fsm_init(fsm_t *handle, const void (*first_fn)(void));
void      fsm_loop(fsm_t *handle);
void      fsm_next(fsm_t *handle, const void (*next_fn)(void), uint32_t delay_ms);
uint32_t  fsm_time(fsm_t *handle);

/*************************************************************************************************/
/** End of File **/
/*************************************************************************************************/

#ifdef __cplusplus
}
#endif
#endif /* _FSM_H_ */

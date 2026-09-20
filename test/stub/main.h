/**
 * @file        main.h
 * @brief       Host stub standing in for the CubeMX generated main.h.
 * @version     2.0.0
 *
 * @author      Nima Askari (NimaLTD)
 * @email       nima.askari@gmail.com
 * @github      https://www.github.com/nimaltd
 *
 * @copyright   (c) 2026 Nima Askari (NimaLTD)
 *              SPDX-License-Identifier: Apache-2.0
 *              See LICENSE.md in the project root for the full license text.
 *
 * @note        This file exists only so the library can be compiled and tested
 *              on a PC. It is not part of the shipped library, and it is never
 *              on the include path of a real STM32 build.
 */

#ifndef MAIN_H
#define MAIN_H

/*
 * ****************************************************************************************************
 * Includes
 * ****************************************************************************************************
*/

#include <stdint.h>

/*
 * ****************************************************************************************************
 * Macros
 * ****************************************************************************************************
*/

/* CMSIS spells volatile this way. */
#define __IO                volatile

/* The HAL compiles this out unless USE_FULL_ASSERT is set, so the stub matches
   the configuration the library ships in. */
#define assert_param(expr)  ((void)0U)

/* The tests are single threaded, so ordering needs no barrier here. */
#define __DMB()

/* PRIMASK, modelled closely enough that a test can tell whether the library
   really closed the window an interrupt could arrive through. The fake
   interrupt in the tests checks fsm_test_irq_enabled() before it fires, which
   is exactly what the hardware does. */
extern int fsm_test_primask;

/* Restoring PRIMASK goes through a function, because re-enabling interrupts is
   the moment a pending one actually fires, and the tests have to model that. */
extern void fsm_test_set_primask(uint32_t value);

#define __get_PRIMASK()   ((uint32_t)fsm_test_primask)
#define __set_PRIMASK(x)  fsm_test_set_primask((uint32_t)(x))
#define __disable_irq()   (fsm_test_primask = 1)
#define __enable_irq()    fsm_test_set_primask(0U)

/* Fires where a higher priority interrupt could land. Compiled out of a real
   build, so it costs nothing on the target. */
extern void fsm_test_hook(void);
#define FSM_TEST_HOOK() fsm_test_hook()

/*
 * ****************************************************************************************************
 * Public function prototypes
 * ****************************************************************************************************
*/

/*****************************************************************************************************/
/**
 * @brief Return the current tick. Backed by a value the tests control.
 */
uint32_t HAL_GetTick(void);

#endif /* MAIN_H */

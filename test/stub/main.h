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
#define __DMB()             ((void)0U)

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

/**
 * @file        fsm_config_template.h
 * @brief       Build time configuration for the fsm library.
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
 *
 * @note        This is the template. Copy it next to the library as
 *              "fsm_config.h" and edit that copy, which the installer does for
 *              you. The name differs from the one fsm.h includes on purpose,
 *              so the template can sit beside the sources without being picked
 *              up as a real configuration.
 */

#ifndef FSM_CONFIG_H
#define FSM_CONFIG_H

/*
 * ****************************************************************************************************
 * Configuration
 * ****************************************************************************************************
*/

/* USER CODE BEGIN FSM_CONFIGURATION */

/* Slots in the task queue. One slot is always kept free so a full queue can be
   told apart from an empty one, so the queue holds FSM_MAX_TASKS - 1 tasks.
   Must be at least 2. */
#define FSM_MAX_TASKS       16U

/* USER CODE END FSM_CONFIGURATION */

#endif /* FSM_CONFIG_H */

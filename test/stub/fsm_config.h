/**
 * @file        fsm_config.h
 * @brief       Library configuration used by the host tests.
 * @version     2.0.0
 *
 * @author      Nima Askari (NimaLTD)
 * @email       nima.askari@gmail.com
 * @github      https://www.github.com/nimaltd
 *
 * @copyright   (c) 2026 Nima Askari (NimaLTD)
 *              SPDX-License-Identifier: Apache-2.0
 *              See LICENSE.md in the project root for the full license text.
 */

#ifndef FSM_CONFIG_H
#define FSM_CONFIG_H

/*
 * ****************************************************************************************************
 * Configuration
 * ****************************************************************************************************
*/

/* Deliberately small. With four slots the queue holds three tasks, so a test
   can fill it and check the full case without queueing sixteen times. */
#define FSM_MAX_TASKS       4U

#endif /* FSM_CONFIG_H */

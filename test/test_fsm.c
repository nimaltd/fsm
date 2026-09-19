/**
 * @file        test_fsm.c
 * @brief       Host unit tests for the fsm library.
 * @version     2.0.0
 *
 * @author      Nima Askari - NimaLTD
 * @github      https://www.github.com/nimaltd
 *
 * @copyright   (c) 2026 Nima Askari - NimaLTD
 *              SPDX-License-Identifier: Apache-2.0
 *              See LICENSE.md in the project root for the full license text.
 *
 * @note        Runs on a PC, not on hardware. Time is a variable the tests set
 *              by hand, so delays are exercised without waiting for them.
 */

/*
 * ****************************************************************************************************
 * Includes
 * ****************************************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>

#include "fsm.h"
#include "main.h"

/*
 * ****************************************************************************************************
 * Macros
 * ****************************************************************************************************
*/

#define CHECK(cond)         test_check((cond), #cond, __LINE__)

/*
 * ****************************************************************************************************
 * Global variables
 * ****************************************************************************************************
*/

static uint32_t test_tick        = 0U;
static int      test_failures    = 0;
static int      test_checks      = 0;
static int      state_a_calls    = 0;
static int      state_b_calls    = 0;
static int      task_calls       = 0;

static fsm_t    test_fsm;

/*
 * ****************************************************************************************************
 * Private function prototypes
 * ****************************************************************************************************
*/

/*****************************************************************************************************/
/**
 * @brief Record one assertion and report it if it failed.
 */
static void test_check(int passed, const char *expression, int line);

/*****************************************************************************************************/
/**
 * @brief Reset the call counters before a test.
 */
static void test_reset(void);

/*****************************************************************************************************/
/**
 * @brief Empty the task queue so a test starts from a known state.
 */
static void queue_drain(void);

/*****************************************************************************************************/
/**
 * @brief A state that counts how often it ran.
 */
static void state_a(void);

/*****************************************************************************************************/
/**
 * @brief A second state, so transitions can be observed.
 */
static void state_b(void);

/*****************************************************************************************************/
/**
 * @brief A state that does nothing, used while draining the queue.
 */
static void state_noop(void);

/*****************************************************************************************************/
/**
 * @brief A queued task that counts how often it ran.
 */
static void task_one(void);

/*
 * ****************************************************************************************************
 * Public function implementations
 * ****************************************************************************************************
*/

/*****************************************************************************************************/
/**
 * @brief Return the tick the tests have set.
 *
 * @return The current fake tick, in milliseconds.
 */
uint32_t HAL_GetTick(void)
{
    return test_tick;
}

/*****************************************************************************************************/
/**
 * @brief Run every test and report the result.
 *
 * @return EXIT_SUCCESS if all assertions passed, EXIT_FAILURE otherwise.
 */
int main(void)
{
    /* The first state runs on the next loop, with no delay. */
    test_reset();
    test_tick = 100U;
    fsm_init(&test_fsm, state_a);
    fsm_loop(&test_fsm);
    CHECK(state_a_calls == 1);

    /* A delayed state waits, then runs once the delay has fully elapsed. */
    test_reset();
    test_tick = 1000U;
    fsm_init(&test_fsm, state_a);
    fsm_next(&test_fsm, state_b, 200U);
    fsm_loop(&test_fsm);
    CHECK(state_b_calls == 0);
    test_tick = 1199U;
    fsm_loop(&test_fsm);
    CHECK(state_b_calls == 0);
    test_tick = 1200U;
    fsm_loop(&test_fsm);
    CHECK(state_b_calls == 1);

    /* The delay clears itself, so the state keeps running afterwards. */
    test_tick = 1201U;
    fsm_loop(&test_fsm);
    CHECK(state_b_calls == 2);

    /* Elapsed time is measured from when the state was entered. */
    test_reset();
    test_tick = 500U;
    fsm_init(&test_fsm, state_a);
    test_tick = 650U;
    CHECK(fsm_time(&test_fsm) == 150U);

    /* A queued task runs on the next loop. */
    queue_drain();
    test_reset();
    fsm_init(&test_fsm, state_a);
    CHECK(fsm_task_add(task_one) == FSM_ERR_NONE);
    fsm_loop(&test_fsm);
    CHECK(task_calls == 1);

    /* The queue holds FSM_MAX_TASKS - 1 tasks, then refuses more. */
    queue_drain();
    test_reset();
    CHECK(fsm_task_add(task_one) == FSM_ERR_NONE);
    CHECK(fsm_task_add(task_one) == FSM_ERR_NONE);
    CHECK(fsm_task_add(task_one) == FSM_ERR_NONE);
    CHECK(fsm_task_add(task_one) == FSM_ERR_FULL);

    /* One task runs per loop, so a burst cannot starve the state machine. */
    fsm_init(&test_fsm, state_a);
    fsm_loop(&test_fsm);
    CHECK(task_calls == 1);
    fsm_loop(&test_fsm);
    CHECK(task_calls == 2);
    fsm_loop(&test_fsm);
    CHECK(task_calls == 3);
    fsm_loop(&test_fsm);
    CHECK(task_calls == 3);

    /* Head and tail wrap around without losing or repeating a task. */
    queue_drain();
    test_reset();
    fsm_init(&test_fsm, state_a);
    for (int i = 0; i < 20; i++)
    {
        CHECK(fsm_task_add(task_one) == FSM_ERR_NONE);
        fsm_loop(&test_fsm);
    }
    CHECK(task_calls == 20);

    /* NULL arguments are refused instead of crashing. */
    test_reset();
    CHECK(fsm_task_add(NULL) == FSM_ERR_FULL);
    CHECK(fsm_time(NULL) == 0U);
    fsm_init(NULL, state_a);
    fsm_loop(NULL);
    fsm_next(NULL, state_a, 0U);
    CHECK(state_a_calls == 0);

    printf("%d checks, %d failed\n", test_checks, test_failures);

    return (test_failures == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*
 * ****************************************************************************************************
 * Private function implementations
 * ****************************************************************************************************
*/

/*****************************************************************************************************/
/**
 * @brief Record one assertion and report it if it failed.
 *
 * @param[in] passed      Non-zero if the assertion held.
 * @param[in] expression  The assertion text, for the failure message.
 * @param[in] line        Line the assertion sits on.
 */
static void test_check(int passed, const char *expression, int line)
{
    test_checks++;

    if (passed == 0)
    {
        test_failures++;
        printf("FAIL line %d: %s\n", line, expression);
    }
}

/*****************************************************************************************************/
/**
 * @brief Reset the call counters before a test.
 */
static void test_reset(void)
{
    state_a_calls = 0;
    state_b_calls = 0;
    task_calls    = 0;
}

/*****************************************************************************************************/
/**
 * @brief Empty the task queue so a test starts from a known state.
 */
static void queue_drain(void)
{
    fsm_t scratch;

    fsm_init(&scratch, state_noop);

    for (uint32_t i = 0U; i < (FSM_MAX_TASKS + 1U); i++)
    {
        fsm_loop(&scratch);
    }
}

/*****************************************************************************************************/
/**
 * @brief A state that counts how often it ran.
 */
static void state_a(void)
{
    state_a_calls++;
}

/*****************************************************************************************************/
/**
 * @brief A second state, so transitions can be observed.
 */
static void state_b(void)
{
    state_b_calls++;
}

/*****************************************************************************************************/
/**
 * @brief A state that does nothing, used while draining the queue.
 */
static void state_noop(void)
{
}

/*****************************************************************************************************/
/**
 * @brief A queued task that counts how often it ran.
 */
static void task_one(void)
{
    task_calls++;
}

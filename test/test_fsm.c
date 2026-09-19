/**
 * @file        test_fsm.c
 * @brief       Host unit tests for the fsm library, built on Unity.
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
 * @note        Runs on a PC, not on hardware. The tick is a variable these
 *              tests set by hand, so a delay is checked instantly rather than
 *              waited out.
 */

/*
 * ****************************************************************************************************
 * Includes
 * ****************************************************************************************************
*/

#include "unity.h"

#include "fsm.h"
#include "main.h"

/*
 * ****************************************************************************************************
 * Global variables
 * ****************************************************************************************************
*/

static uint32_t test_tick     = 0U;
static int      state_a_calls = 0;
static int      state_b_calls = 0;
static int      task_calls    = 0;

static fsm_t    test_fsm;

/*
 * ****************************************************************************************************
 * Private function prototypes
 * ****************************************************************************************************
*/

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
 * @brief Put the library back to a known state before every test.
 *
 * The task queue lives in a file scope variable inside fsm.c, so anything a
 * previous test left queued would otherwise leak into the next one.
 */
void setUp(void)
{
    test_tick = 0U;

    queue_drain();

    /* Draining runs whatever was left queued, so the counters reset after it. */
    state_a_calls = 0;
    state_b_calls = 0;
    task_calls    = 0;
}

/*****************************************************************************************************/
/**
 * @brief Required by Unity. Nothing to undo after a test.
 */
void tearDown(void)
{
}

/*****************************************************************************************************/
/**
 * @brief The first state runs on the next loop, with no delay.
 */
void test_init_runs_first_state(void)
{
    test_tick = 100U;
    fsm_init(&test_fsm, state_a);
    fsm_loop(&test_fsm);

    TEST_ASSERT_EQUAL_INT(1, state_a_calls);
}

/*****************************************************************************************************/
/**
 * @brief A delayed state waits, then runs once the delay has fully elapsed.
 */
void test_next_waits_for_the_delay(void)
{
    test_tick = 1000U;
    fsm_init(&test_fsm, state_a);
    fsm_next(&test_fsm, state_b, 200U);

    fsm_loop(&test_fsm);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, state_b_calls, "ran before the delay started");

    test_tick = 1199U;
    fsm_loop(&test_fsm);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, state_b_calls, "ran one millisecond early");

    test_tick = 1200U;
    fsm_loop(&test_fsm);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, state_b_calls, "did not run when the delay expired");
}

/*****************************************************************************************************/
/**
 * @brief The delay clears itself, so the state keeps running afterwards.
 */
void test_delay_clears_after_running(void)
{
    test_tick = 1000U;
    fsm_init(&test_fsm, state_a);
    fsm_next(&test_fsm, state_b, 200U);

    test_tick = 1200U;
    fsm_loop(&test_fsm);
    test_tick = 1201U;
    fsm_loop(&test_fsm);

    TEST_ASSERT_EQUAL_INT(2, state_b_calls);
}

/*****************************************************************************************************/
/**
 * @brief Elapsed time is measured from when the state was entered.
 */
void test_time_counts_from_state_entry(void)
{
    test_tick = 500U;
    fsm_init(&test_fsm, state_a);

    test_tick = 650U;

    TEST_ASSERT_EQUAL_UINT32(150U, fsm_time(&test_fsm));
}

/*****************************************************************************************************/
/**
 * @brief A queued task runs on the next loop.
 */
void test_queued_task_runs(void)
{
    fsm_init(&test_fsm, state_a);

    TEST_ASSERT_EQUAL_INT(FSM_ERR_NONE, fsm_task_add(task_one));

    fsm_loop(&test_fsm);

    TEST_ASSERT_EQUAL_INT(1, task_calls);
}

/*****************************************************************************************************/
/**
 * @brief The queue holds FSM_MAX_TASKS - 1 tasks, then refuses more.
 */
void test_queue_refuses_when_full(void)
{
    TEST_ASSERT_EQUAL_INT(FSM_ERR_NONE, fsm_task_add(task_one));
    TEST_ASSERT_EQUAL_INT(FSM_ERR_NONE, fsm_task_add(task_one));
    TEST_ASSERT_EQUAL_INT(FSM_ERR_NONE, fsm_task_add(task_one));

    TEST_ASSERT_EQUAL_INT_MESSAGE(FSM_ERR_FULL, fsm_task_add(task_one),
                                  "accepted more than FSM_MAX_TASKS - 1 tasks");
}

/*****************************************************************************************************/
/**
 * @brief One task runs per loop, so a burst cannot starve the state machine.
 */
void test_one_task_runs_per_loop(void)
{
    fsm_init(&test_fsm, state_a);

    (void)fsm_task_add(task_one);
    (void)fsm_task_add(task_one);
    (void)fsm_task_add(task_one);

    fsm_loop(&test_fsm);
    TEST_ASSERT_EQUAL_INT(1, task_calls);

    fsm_loop(&test_fsm);
    TEST_ASSERT_EQUAL_INT(2, task_calls);

    fsm_loop(&test_fsm);
    TEST_ASSERT_EQUAL_INT(3, task_calls);

    fsm_loop(&test_fsm);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, task_calls, "ran a task that was never queued");
}

/*****************************************************************************************************/
/**
 * @brief Head and tail wrap around without losing or repeating a task.
 */
void test_queue_wraps_around(void)
{
    fsm_init(&test_fsm, state_a);

    for (int i = 0; i < 20; i++)
    {
        TEST_ASSERT_EQUAL_INT(FSM_ERR_NONE, fsm_task_add(task_one));
        fsm_loop(&test_fsm);
    }

    TEST_ASSERT_EQUAL_INT(20, task_calls);
}

/*****************************************************************************************************/
/**
 * @brief NULL arguments are refused instead of crashing.
 */
void test_null_arguments_are_refused(void)
{
    TEST_ASSERT_EQUAL_INT(FSM_ERR_FULL, fsm_task_add(NULL));
    TEST_ASSERT_EQUAL_UINT32(0U, fsm_time(NULL));

    fsm_init(NULL, state_a);
    fsm_loop(NULL);
    fsm_next(NULL, state_a, 0U);

    TEST_ASSERT_EQUAL_INT(0, state_a_calls);
}

/*****************************************************************************************************/
/**
 * @brief Run every test and report the result.
 *
 * @return 0 if every test passed, otherwise the number of failures.
 */
int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_init_runs_first_state);
    RUN_TEST(test_next_waits_for_the_delay);
    RUN_TEST(test_delay_clears_after_running);
    RUN_TEST(test_time_counts_from_state_entry);
    RUN_TEST(test_queued_task_runs);
    RUN_TEST(test_queue_refuses_when_full);
    RUN_TEST(test_one_task_runs_per_loop);
    RUN_TEST(test_queue_wraps_around);
    RUN_TEST(test_null_arguments_are_refused);

    return UNITY_END();
}

/*
 * ****************************************************************************************************
 * Private function implementations
 * ****************************************************************************************************
*/

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

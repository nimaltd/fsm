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

static uint32_t test_tick      = 0U;
static int      state_a_calls  = 0;
static int      state_b_calls  = 0;
static int      task_calls     = 0;
static int      task_two_calls = 0;

static fsm_t    test_fsm;

/* Set by a test to have the fake interrupt queue this, once, from inside
   fsm_task_add. NULL means no interrupt arrives. */
static fsm_fn_t queue_preempt_with = NULL;

/* Stands in for the real PRIMASK. Zero means interrupts are enabled. */
int fsm_test_primask = 0;

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

/*****************************************************************************************************/
/**
 * @brief A second queued task, so two can be told apart.
 */
static void task_two(void);

/*****************************************************************************************************/
/**
 * @brief A state that gives up after five seconds, the way a real one would.
 */
static void state_waits_then_times_out(void);

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
    state_a_calls      = 0;
    state_b_calls      = 0;
    task_calls         = 0;
    task_two_calls     = 0;
    queue_preempt_with = NULL;
    fsm_test_primask   = 0;
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
 * @brief Elapsed time keeps growing while a state stays put.
 *
 * The header promises time since the state was entered. A state with no delay
 * runs on every pass of the main loop, and this is the case that matters,
 * because it is the one every timeout is built on.
 */
void test_time_grows_while_a_state_runs(void)
{
    test_tick = 1000U;
    fsm_init(&test_fsm, state_a);

    for (uint32_t i = 0U; i < 5000U; i++)
    {
        fsm_loop(&test_fsm);
        test_tick++;
    }

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(5000U, fsm_time(&test_fsm),
                                     "time in state stopped counting once the state ran");
}

/*****************************************************************************************************/
/**
 * @brief The timeout pattern the whole library exists to support.
 *
 * Written the way a user writes it, not the way the code happens to work.
 */
void test_a_timeout_actually_fires(void)
{
    uint32_t fired_at = 0U;

    test_tick = 0U;
    fsm_init(&test_fsm, state_waits_then_times_out);

    for (uint32_t i = 0U; i < 6000U; i++)
    {
        fsm_loop(&test_fsm);

        if ((state_b_calls > 0) && (fired_at == 0U))
        {
            fired_at = test_tick;
        }

        test_tick++;
    }

    TEST_ASSERT_MESSAGE(state_b_calls > 0, "a five second timeout never fired");
    TEST_ASSERT_UINT32_WITHIN_MESSAGE(2U, 5001U, fired_at, "the timeout fired at the wrong time");
}

/*****************************************************************************************************/
/**
 * @brief Elapsed time restarts when the state actually changes.
 */
void test_time_restarts_on_a_real_transition(void)
{
    test_tick = 1000U;
    fsm_init(&test_fsm, state_a);
    fsm_loop(&test_fsm);

    test_tick = 1500U;
    fsm_loop(&test_fsm);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(500U, fsm_time(&test_fsm),
                                     "time should keep counting inside one state");

    fsm_next(&test_fsm, state_b, 0U);

    test_tick = 1600U;
    fsm_loop(&test_fsm);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0U, fsm_time(&test_fsm),
                                     "time should start again on entering a new state");

    test_tick = 1700U;
    TEST_ASSERT_EQUAL_UINT32(100U, fsm_time(&test_fsm));
}

/*****************************************************************************************************/
/**
 * @brief A delay is time spent waiting to enter, not time spent in the state.
 *
 * So a state scheduled with fsm_next(..., 200) sees an elapsed time of zero on
 * its first run, not two hundred.
 */
void test_a_delay_does_not_count_as_time_in_the_state(void)
{
    test_tick = 1000U;
    fsm_init(&test_fsm, state_a);
    fsm_loop(&test_fsm);

    fsm_next(&test_fsm, state_b, 200U);

    test_tick = 1200U;
    fsm_loop(&test_fsm);

    TEST_ASSERT_EQUAL_UINT32(0U, fsm_time(&test_fsm));
}

/*****************************************************************************************************/
/**
 * @brief Two interrupts queueing at once must not lose one another's work.
 *
 * The hook fires where a higher priority interrupt would land, and the fake
 * interrupt only preempts when interrupts are actually enabled, which is what
 * the hardware does. Before this was fixed, one task vanished without any
 * error being returned.
 */
void test_two_interrupts_do_not_lose_a_task(void)
{
    queue_preempt_with = task_two;

    TEST_ASSERT_EQUAL_INT(FSM_ERR_NONE, fsm_task_add(task_one));

    queue_preempt_with = NULL;

    fsm_init(&test_fsm, state_noop);

    for (uint32_t i = 0U; i < (FSM_MAX_TASKS + 2U); i++)
    {
        fsm_loop(&test_fsm);
    }

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, task_calls, "the first task was lost");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, task_two_calls, "the preempting task was lost");
}

/*****************************************************************************************************/
/**
 * @brief A stopped machine runs nothing, and says so.
 */
void test_stop_halts_the_machine(void)
{
    fsm_init(&test_fsm, state_a);
    fsm_loop(&test_fsm);
    TEST_ASSERT_TRUE(fsm_running(&test_fsm));

    fsm_stop(&test_fsm);
    TEST_ASSERT_FALSE(fsm_running(&test_fsm));

    fsm_loop(&test_fsm);
    fsm_loop(&test_fsm);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, state_a_calls, "a stopped machine kept running");
}

/*****************************************************************************************************/
/**
 * @brief A stopped machine starts again when given a new state.
 */
void test_a_stopped_machine_can_be_restarted(void)
{
    fsm_init(&test_fsm, state_a);
    fsm_stop(&test_fsm);

    fsm_next(&test_fsm, state_b, 0U);
    fsm_loop(&test_fsm);

    TEST_ASSERT_TRUE(fsm_running(&test_fsm));
    TEST_ASSERT_EQUAL_INT(1, state_b_calls);
}

/*****************************************************************************************************/
/**
 * @brief Queued tasks still run while the machine is stopped.
 *
 * The queue belongs to the application, not to any one machine, so stopping a
 * machine must not quietly stop the work an interrupt handed over.
 */
void test_a_stopped_machine_still_serves_the_queue(void)
{
    fsm_init(&test_fsm, state_a);
    fsm_stop(&test_fsm);

    TEST_ASSERT_EQUAL_INT(FSM_ERR_NONE, fsm_task_add(task_one));
    fsm_loop(&test_fsm);

    TEST_ASSERT_EQUAL_INT(1, task_calls);
}

/*****************************************************************************************************/
/**
 * @brief The peak depth is what FSM_MAX_TASKS should be sized against.
 */
void test_the_queue_reports_how_deep_it_ever_got(void)
{
    uint32_t before = fsm_task_peak();

    (void)fsm_task_add(task_one);
    (void)fsm_task_add(task_one);

    TEST_ASSERT_MESSAGE(fsm_task_peak() >= 2U, "peak depth was not recorded");
    TEST_ASSERT_MESSAGE(fsm_task_peak() >= before, "peak depth went backwards");
}

/*****************************************************************************************************/
/**
 * @brief Flushing drops everything queued.
 */
void test_flush_empties_the_queue(void)
{
    fsm_init(&test_fsm, state_noop);

    (void)fsm_task_add(task_one);
    (void)fsm_task_add(task_one);

    fsm_task_flush();

    for (uint32_t i = 0U; i < (FSM_MAX_TASKS + 2U); i++)
    {
        fsm_loop(&test_fsm);
    }

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, task_calls, "a flushed task still ran");
}

/*****************************************************************************************************/
/**
 * @brief Stopping and reading a NULL handle is refused, not followed.
 */
void test_null_is_refused_by_the_new_calls(void)
{
    fsm_stop(NULL);

    TEST_ASSERT_FALSE(fsm_running(NULL));
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
    RUN_TEST(test_time_grows_while_a_state_runs);
    RUN_TEST(test_a_timeout_actually_fires);
    RUN_TEST(test_time_restarts_on_a_real_transition);
    RUN_TEST(test_a_delay_does_not_count_as_time_in_the_state);
    RUN_TEST(test_two_interrupts_do_not_lose_a_task);
    RUN_TEST(test_stop_halts_the_machine);
    RUN_TEST(test_a_stopped_machine_can_be_restarted);
    RUN_TEST(test_a_stopped_machine_still_serves_the_queue);
    RUN_TEST(test_the_queue_reports_how_deep_it_ever_got);
    RUN_TEST(test_flush_empties_the_queue);
    RUN_TEST(test_null_is_refused_by_the_new_calls);

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

/*****************************************************************************************************/
/**
 * @brief A second queued task, so two can be told apart.
 */
static void task_two(void)
{
    task_two_calls++;
}

/*****************************************************************************************************/
/**
 * @brief A state that gives up after five seconds, the way a real one would.
 */
static void state_waits_then_times_out(void)
{
    if (fsm_time(&test_fsm) > 5000U)
    {
        fsm_next(&test_fsm, state_b, 0U);
    }
}

/*****************************************************************************************************/
/**
 * @brief Stands in for a higher priority interrupt arriving mid update.
 *
 * It only fires when interrupts are enabled, because that is the one thing the
 * hardware guarantees and the only thing the fix can rely on.
 */
void fsm_test_hook(void)
{
    if ((queue_preempt_with != NULL) && (fsm_test_primask == 0))
    {
        fsm_fn_t pending = queue_preempt_with;

        queue_preempt_with = NULL;
        (void)fsm_task_add(pending);
    }
}

/*****************************************************************************************************/
/**
 * @brief Restore PRIMASK, and let any interrupt held off by it through.
 *
 * On the hardware an interrupt raised while PRIMASK is set is not lost, it is
 * held pending and taken the moment interrupts are enabled again. Modelling
 * that is what makes the test meaningful: the fix should not drop the
 * interrupt, only delay it until the queue is consistent again.
 *
 * @param[in] value  The PRIMASK value being restored.
 */
void fsm_test_set_primask(uint32_t value)
{
    fsm_test_primask = (int)value;

    if ((fsm_test_primask == 0) && (queue_preempt_with != NULL))
    {
        fsm_fn_t pending = queue_preempt_with;

        queue_preempt_with = NULL;
        (void)fsm_task_add(pending);
    }
}

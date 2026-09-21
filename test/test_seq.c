/**
 * @file        test_seq.c
 * @brief       Host unit tests for the sequencer library, built on Unity.
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

#include "seq.h"
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
static int      requeue_limit  = 0;
static int      task_total     = 0;

static seq_t    test_seq;

/* What the last task to run was handed, so a test can check it arrived. */
static void *last_task_arg = NULL;

/* What the last state to run was handed. */
static seq_t *last_state_handle = NULL;

/* Set by a test to have the fake interrupt queue this, once, from inside
   seq_task_add. NULL means no interrupt arrives. */
static seq_task_fn_t queue_preempt_with = NULL;

/* Stands in for the real PRIMASK. Zero means interrupts are enabled. */
int seq_test_primask = 0;

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
 * @brief A state that counts how often it ran and remembers its handle.
 */
static void state_a(seq_t *handle);

/*****************************************************************************************************/
/**
 * @brief A second state, so transitions can be observed.
 */
static void state_b(seq_t *handle);

/*****************************************************************************************************/
/**
 * @brief A state that does nothing, used while draining the queue.
 */
static void state_noop(seq_t *handle);

/*****************************************************************************************************/
/**
 * @brief A state that counts a run against whatever handle->user points at.
 */
static void state_counts_into_user(seq_t *handle);

/*****************************************************************************************************/
/**
 * @brief A queued task that counts how often it ran and remembers its argument.
 */
static void task_one(void *arg);

/*****************************************************************************************************/
/**
 * @brief A second queued task, so two can be told apart.
 */
static void task_two(void *arg);

/*****************************************************************************************************/
/**
 * @brief A task that queues itself again, up to requeue_limit times.
 */
static void task_requeues(void *arg);

/*****************************************************************************************************/
/**
 * @brief A task that adds what its argument points at to a running total.
 */
static void task_sums(void *arg);

/*****************************************************************************************************/
/**
 * @brief A state that gives up after five seconds, the way a real one would.
 */
static void state_waits_then_times_out(seq_t *handle);

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
 * The task queue lives in a file scope variable inside seq.c, so anything a
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
    requeue_limit      = 0;
    task_total         = 0;
    last_task_arg      = NULL;
    last_state_handle  = NULL;
    queue_preempt_with = NULL;
    seq_test_primask   = 0;
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
    seq_init(&test_seq, state_a, NULL);
    seq_loop(&test_seq);

    TEST_ASSERT_EQUAL_INT(1, state_a_calls);
}

/*****************************************************************************************************/
/**
 * @brief A delayed state waits, then runs once the delay has fully elapsed.
 */
void test_next_waits_for_the_delay(void)
{
    test_tick = 1000U;
    seq_init(&test_seq, state_a, NULL);
    seq_next(&test_seq, state_b, 200U);

    seq_loop(&test_seq);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, state_b_calls, "ran before the delay started");

    test_tick = 1199U;
    seq_loop(&test_seq);
    TEST_ASSERT_EQUAL_INT_MESSAGE(0, state_b_calls, "ran one millisecond early");

    test_tick = 1200U;
    seq_loop(&test_seq);
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, state_b_calls, "did not run when the delay expired");
}

/*****************************************************************************************************/
/**
 * @brief The delay clears itself, so the state keeps running afterwards.
 */
void test_delay_clears_after_running(void)
{
    test_tick = 1000U;
    seq_init(&test_seq, state_a, NULL);
    seq_next(&test_seq, state_b, 200U);

    test_tick = 1200U;
    seq_loop(&test_seq);
    test_tick = 1201U;
    seq_loop(&test_seq);

    TEST_ASSERT_EQUAL_INT(2, state_b_calls);
}

/*****************************************************************************************************/
/**
 * @brief Elapsed time is measured from when the state was entered.
 */
void test_time_counts_from_state_entry(void)
{
    test_tick = 500U;
    seq_init(&test_seq, state_a, NULL);

    test_tick = 650U;

    TEST_ASSERT_EQUAL_UINT32(150U, seq_time(&test_seq));
}

/*****************************************************************************************************/
/**
 * @brief A queued task runs on the next loop.
 */
void test_queued_task_runs(void)
{
    seq_init(&test_seq, state_a, NULL);

    TEST_ASSERT_EQUAL_INT(SEQ_ERR_NONE, seq_task_add(task_one, NULL));

    seq_loop(&test_seq);

    TEST_ASSERT_EQUAL_INT(1, task_calls);
}

/*****************************************************************************************************/
/**
 * @brief A state is handed the handle it belongs to.
 *
 * Without it a state function has to name its machine through a file scope
 * variable, which is what stops one set of states from driving two machines.
 */
void test_a_state_is_given_its_own_handle(void)
{
    seq_init(&test_seq, state_a, NULL);
    seq_loop(&test_seq);

    TEST_ASSERT_EQUAL_PTR_MESSAGE(&test_seq, last_state_handle,
                                  "the state did not get the handle it belongs to");
}

/*****************************************************************************************************/
/**
 * @brief One set of state functions drives two machines at once.
 *
 * This is the whole reason the handle is passed in. Each machine counts into
 * its own total, through the pointer given to seq_init().
 */
void test_two_machines_share_one_state_function(void)
{
    seq_t first;
    seq_t second;
    int   first_runs  = 0;
    int   second_runs = 0;

    seq_init(&first, state_counts_into_user, &first_runs);
    seq_init(&second, state_counts_into_user, &second_runs);

    seq_loop(&first);
    seq_loop(&first);
    seq_loop(&second);

    TEST_ASSERT_EQUAL_INT_MESSAGE(2, first_runs, "the first machine counted wrong");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, second_runs, "the second machine counted wrong");
}

/*****************************************************************************************************/
/**
 * @brief A task is handed the argument it was queued with.
 */
void test_a_task_is_given_its_argument(void)
{
    int marker = 0;

    seq_init(&test_seq, state_noop, NULL);

    TEST_ASSERT_EQUAL_INT(SEQ_ERR_NONE, seq_task_add(task_one, &marker));
    seq_loop(&test_seq);

    TEST_ASSERT_EQUAL_PTR_MESSAGE(&marker, last_task_arg, "the argument did not arrive");
}

/*****************************************************************************************************/
/**
 * @brief Each queued task keeps its own argument, in order.
 *
 * One argument stored per queue slot, not one shared by the queue. Sharing it
 * would look right for a single task and go wrong the moment an interrupt
 * queued a second one before the first had run.
 */
void test_every_queued_task_keeps_its_own_argument(void)
{
    /* Powers of ten, so any mix up gives a total that names what went wrong
       rather than one that happens to add up anyway. */
    int one   = 1;
    int two   = 20;
    int three = 300;

    seq_init(&test_seq, state_noop, NULL);

    (void)seq_task_add(task_sums, &one);
    (void)seq_task_add(task_sums, &two);
    (void)seq_task_add(task_sums, &three);

    seq_loop(&test_seq);

    TEST_ASSERT_EQUAL_INT_MESSAGE(3, task_calls, "the burst did not clear in one pass");
    TEST_ASSERT_EQUAL_INT_MESSAGE(321, task_total,
                                  "a task ran with another task's argument");
}

/*****************************************************************************************************/
/**
 * @brief An argument survives the queue wrapping around.
 */
void test_arguments_survive_the_queue_wrapping(void)
{
    int markers[20];

    seq_init(&test_seq, state_noop, NULL);

    for (int i = 0; i < 20; i++)
    {
        markers[i] = i;

        TEST_ASSERT_EQUAL_INT(SEQ_ERR_NONE, seq_task_add(task_one, &markers[i]));
        seq_loop(&test_seq);

        TEST_ASSERT_EQUAL_PTR_MESSAGE(&markers[i], last_task_arg,
                                      "an argument was lost as the queue wrapped");
    }

    TEST_ASSERT_EQUAL_INT(20, task_calls);
}

/*****************************************************************************************************/
/**
 * @brief The queue holds SEQ_MAX_TASKS - 1 tasks, then refuses more.
 */
void test_queue_refuses_when_full(void)
{
    /* Written against SEQ_MAX_TASKS rather than a fixed number, so the tests do
       not need a configuration of their own just to keep the queue small. */
    for (uint32_t i = 0U; i < (SEQ_MAX_TASKS - 1U); i++)
    {
        TEST_ASSERT_EQUAL_INT_MESSAGE(SEQ_ERR_NONE, seq_task_add(task_one, NULL),
                                      "refused a task while the queue had room");
    }

    TEST_ASSERT_EQUAL_INT_MESSAGE(SEQ_ERR_FULL, seq_task_add(task_one, NULL),
                                  "accepted more than SEQ_MAX_TASKS - 1 tasks");
}

/*****************************************************************************************************/
/**
 * @brief A whole burst is cleared in one pass.
 *
 * The last task of a burst should not have to wait one loop iteration per task
 * ahead of it.
 */
void test_a_burst_runs_in_one_loop(void)
{
    seq_init(&test_seq, state_a, NULL);

    (void)seq_task_add(task_one, NULL);
    (void)seq_task_add(task_one, NULL);
    (void)seq_task_add(task_one, NULL);

    seq_loop(&test_seq);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, task_calls, "the burst was not cleared in one pass");

    seq_loop(&test_seq);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, task_calls, "ran a task that was never queued");
}

/*****************************************************************************************************/
/**
 * @brief Head and tail wrap around without losing or repeating a task.
 */
void test_queue_wraps_around(void)
{
    seq_init(&test_seq, state_a, NULL);

    for (int i = 0; i < 20; i++)
    {
        TEST_ASSERT_EQUAL_INT(SEQ_ERR_NONE, seq_task_add(task_one, NULL));
        seq_loop(&test_seq);
    }

    TEST_ASSERT_EQUAL_INT(20, task_calls);
}

/*****************************************************************************************************/
/**
 * @brief NULL arguments are refused instead of crashing.
 */
void test_null_arguments_are_refused(void)
{
    TEST_ASSERT_EQUAL_INT(SEQ_ERR_INVALID, seq_task_add(NULL, NULL));
    TEST_ASSERT_EQUAL_UINT32(0U, seq_time(NULL));

    seq_init(NULL, state_a, NULL);
    seq_loop(NULL);
    seq_next(NULL, state_a, 0U);

    TEST_ASSERT_EQUAL_INT(0, state_a_calls);
}

/*****************************************************************************************************/
/**
 * @brief A NULL user pointer is allowed, since not every machine needs one.
 */
void test_a_null_user_pointer_is_fine(void)
{
    seq_init(&test_seq, state_a, NULL);
    seq_loop(&test_seq);

    TEST_ASSERT_EQUAL_INT(1, state_a_calls);
    TEST_ASSERT_NULL(test_seq.user);
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
    seq_init(&test_seq, state_a, NULL);

    for (uint32_t i = 0U; i < 5000U; i++)
    {
        seq_loop(&test_seq);
        test_tick++;
    }

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(5000U, seq_time(&test_seq),
                                     "time in state stopped counting once the state ran");
}

/*****************************************************************************************************/
/**
 * @brief The timeout pattern the whole library exists to support.
 *
 * Written the way a user writes it, not the way the code happens to work. The
 * state reads the clock off the handle it was given, with nothing at file
 * scope, which is how a state function is meant to look now.
 */
void test_a_timeout_actually_fires(void)
{
    uint32_t fired_at = 0U;

    test_tick = 0U;
    seq_init(&test_seq, state_waits_then_times_out, NULL);

    for (uint32_t i = 0U; i < 6000U; i++)
    {
        seq_loop(&test_seq);

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
    seq_init(&test_seq, state_a, NULL);
    seq_loop(&test_seq);

    test_tick = 1500U;
    seq_loop(&test_seq);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(500U, seq_time(&test_seq),
                                     "time should keep counting inside one state");

    seq_next(&test_seq, state_b, 0U);

    test_tick = 1600U;
    seq_loop(&test_seq);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0U, seq_time(&test_seq),
                                     "time should start again on entering a new state");

    test_tick = 1700U;
    TEST_ASSERT_EQUAL_UINT32(100U, seq_time(&test_seq));
}

/*****************************************************************************************************/
/**
 * @brief A delay is time spent waiting to enter, not time spent in the state.
 *
 * So a state scheduled with seq_next(..., 200) sees an elapsed time of zero on
 * its first run, not two hundred.
 */
void test_a_delay_does_not_count_as_time_in_the_state(void)
{
    test_tick = 1000U;
    seq_init(&test_seq, state_a, NULL);
    seq_loop(&test_seq);

    seq_next(&test_seq, state_b, 200U);

    test_tick = 1200U;
    seq_loop(&test_seq);

    TEST_ASSERT_EQUAL_UINT32(0U, seq_time(&test_seq));
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

    TEST_ASSERT_EQUAL_INT(SEQ_ERR_NONE, seq_task_add(task_one, NULL));

    queue_preempt_with = NULL;

    seq_init(&test_seq, state_noop, NULL);

    for (uint32_t i = 0U; i < (SEQ_MAX_TASKS + 2U); i++)
    {
        seq_loop(&test_seq);
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
    seq_init(&test_seq, state_a, NULL);
    seq_loop(&test_seq);
    TEST_ASSERT_TRUE(seq_running(&test_seq));

    seq_stop(&test_seq);
    TEST_ASSERT_FALSE(seq_running(&test_seq));

    seq_loop(&test_seq);
    seq_loop(&test_seq);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, state_a_calls, "a stopped machine kept running");
}

/*****************************************************************************************************/
/**
 * @brief A stopped machine starts again when given a new state.
 */
void test_a_stopped_machine_can_be_restarted(void)
{
    seq_init(&test_seq, state_a, NULL);
    seq_stop(&test_seq);

    seq_next(&test_seq, state_b, 0U);
    seq_loop(&test_seq);

    TEST_ASSERT_TRUE(seq_running(&test_seq));
    TEST_ASSERT_EQUAL_INT(1, state_b_calls);
}

/*****************************************************************************************************/
/**
 * @brief Stopping a machine leaves its user pointer alone.
 *
 * It belongs to the caller, not to the run, so restarting must not need it to
 * be handed over a second time.
 */
void test_stopping_keeps_the_user_pointer(void)
{
    int marker = 0;

    seq_init(&test_seq, state_counts_into_user, &marker);
    seq_stop(&test_seq);
    seq_next(&test_seq, state_counts_into_user, 0U);
    seq_loop(&test_seq);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, marker, "the user pointer was lost across a stop");
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
    seq_init(&test_seq, state_a, NULL);
    seq_stop(&test_seq);

    TEST_ASSERT_EQUAL_INT(SEQ_ERR_NONE, seq_task_add(task_one, NULL));
    seq_loop(&test_seq);

    TEST_ASSERT_EQUAL_INT(1, task_calls);
}

/*****************************************************************************************************/
/**
 * @brief The peak depth is what SEQ_MAX_TASKS should be sized against.
 */
void test_the_queue_reports_how_deep_it_ever_got(void)
{
    uint32_t before = seq_task_peak();

    (void)seq_task_add(task_one, NULL);
    (void)seq_task_add(task_one, NULL);

    TEST_ASSERT_MESSAGE(seq_task_peak() >= 2U, "peak depth was not recorded");
    TEST_ASSERT_MESSAGE(seq_task_peak() >= before, "peak depth went backwards");
}

/*****************************************************************************************************/
/**
 * @brief Flushing drops everything queued.
 */
void test_flush_empties_the_queue(void)
{
    seq_init(&test_seq, state_noop, NULL);

    (void)seq_task_add(task_one, NULL);
    (void)seq_task_add(task_one, NULL);

    seq_task_flush();

    for (uint32_t i = 0U; i < (SEQ_MAX_TASKS + 2U); i++)
    {
        seq_loop(&test_seq);
    }

    TEST_ASSERT_EQUAL_INT_MESSAGE(0, task_calls, "a flushed task still ran");
}

/*****************************************************************************************************/
/**
 * @brief Stopping and reading a NULL handle is refused, not followed.
 */
void test_null_is_refused_by_the_new_calls(void)
{
    seq_stop(NULL);

    TEST_ASSERT_FALSE(seq_running(NULL));
}

/*****************************************************************************************************/
/**
 * @brief A NULL task is told apart from a full queue.
 *
 * Both used to return SEQ_ERR_FULL, which said the queue was the problem when
 * the argument was.
 */
void test_a_null_task_is_not_reported_as_a_full_queue(void)
{
    TEST_ASSERT_EQUAL_INT(SEQ_ERR_INVALID, seq_task_add(NULL, NULL));
    TEST_ASSERT_EQUAL_INT_MESSAGE(SEQ_ERR_NONE, seq_task_add(task_one, NULL),
                                  "a real task was refused after a NULL one");
}

/*****************************************************************************************************/
/**
 * @brief A stopped machine reports no elapsed time.
 *
 * It is in no state, so a number counting up from the last transition would
 * only invite a timeout that can never be acted on.
 */
void test_time_is_zero_once_stopped(void)
{
    test_tick = 1000U;
    seq_init(&test_seq, state_a, NULL);
    seq_loop(&test_seq);

    test_tick = 1500U;
    TEST_ASSERT_EQUAL_UINT32(500U, seq_time(&test_seq));

    seq_stop(&test_seq);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(0U, seq_time(&test_seq),
                                     "a stopped machine still reported elapsed time");

    test_tick = 2000U;
    TEST_ASSERT_EQUAL_UINT32(0U, seq_time(&test_seq));
}

/*****************************************************************************************************/
/**
 * @brief A task that queues another does not hold the loop for ever.
 *
 * This is why the drain stops at the queue's end as it was on entry, rather
 * than at empty. Draining to empty would spin here and the state would never
 * run again, which is the one thing the loop exists to do.
 */
void test_a_task_that_requeues_itself_does_not_trap_the_loop(void)
{
    seq_init(&test_seq, state_a, NULL);

    requeue_limit = 5;
    (void)seq_task_add(task_requeues, NULL);

    seq_loop(&test_seq);

    TEST_ASSERT_EQUAL_INT_MESSAGE(1, task_calls, "the requeued task ran in the same pass");
    TEST_ASSERT_EQUAL_INT_MESSAGE(1, state_a_calls, "the state never got its turn");

    seq_loop(&test_seq);
    TEST_ASSERT_EQUAL_INT(2, task_calls);
    TEST_ASSERT_EQUAL_INT(2, state_a_calls);
}

/*****************************************************************************************************/
/**
 * @brief Tasks queued while a burst runs wait for the next pass.
 */
void test_work_queued_during_a_burst_waits(void)
{
    seq_init(&test_seq, state_noop, NULL);

    requeue_limit = 1;
    (void)seq_task_add(task_requeues, NULL);
    (void)seq_task_add(task_one, NULL);

    seq_loop(&test_seq);
    TEST_ASSERT_EQUAL_INT_MESSAGE(2, task_calls, "the first pass ran the wrong number of tasks");

    seq_loop(&test_seq);
    TEST_ASSERT_EQUAL_INT_MESSAGE(3, task_calls, "the task queued during the burst never ran");
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
    RUN_TEST(test_a_state_is_given_its_own_handle);
    RUN_TEST(test_two_machines_share_one_state_function);
    RUN_TEST(test_a_task_is_given_its_argument);
    RUN_TEST(test_every_queued_task_keeps_its_own_argument);
    RUN_TEST(test_arguments_survive_the_queue_wrapping);
    RUN_TEST(test_a_null_user_pointer_is_fine);
    RUN_TEST(test_stopping_keeps_the_user_pointer);
    RUN_TEST(test_queue_refuses_when_full);
    RUN_TEST(test_a_burst_runs_in_one_loop);
    RUN_TEST(test_a_task_that_requeues_itself_does_not_trap_the_loop);
    RUN_TEST(test_work_queued_during_a_burst_waits);
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
    RUN_TEST(test_a_null_task_is_not_reported_as_a_full_queue);
    RUN_TEST(test_time_is_zero_once_stopped);

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
    seq_t scratch;

    seq_init(&scratch, state_noop, NULL);

    for (uint32_t i = 0U; i < (SEQ_MAX_TASKS + 1U); i++)
    {
        seq_loop(&scratch);
    }
}

/*****************************************************************************************************/
/**
 * @brief A state that counts how often it ran and remembers its handle.
 */
static void state_a(seq_t *handle)
{
    last_state_handle = handle;
    state_a_calls++;
}

/*****************************************************************************************************/
/**
 * @brief A second state, so transitions can be observed.
 */
static void state_b(seq_t *handle)
{
    last_state_handle = handle;
    state_b_calls++;
}

/*****************************************************************************************************/
/**
 * @brief A state that does nothing, used while draining the queue.
 */
static void state_noop(seq_t *handle)
{
    (void)handle;
}

/*****************************************************************************************************/
/**
 * @brief A state that counts a run against whatever handle->user points at.
 */
static void state_counts_into_user(seq_t *handle)
{
    /* Guarded so a state handed the wrong thing fails an assertion rather than
       bringing the whole suite down with it. */
    if ((handle != NULL) && (handle->user != NULL))
    {
        (*(int *)handle->user)++;
    }
}

/*****************************************************************************************************/
/**
 * @brief A queued task that counts how often it ran and remembers its argument.
 */
static void task_one(void *arg)
{
    last_task_arg = arg;
    task_calls++;
}

/*****************************************************************************************************/
/**
 * @brief A second queued task, so two can be told apart.
 */
static void task_two(void *arg)
{
    (void)arg;
    task_two_calls++;
}

/*****************************************************************************************************/
/**
 * @brief A task that queues itself again, up to requeue_limit times.
 */
static void task_requeues(void *arg)
{
    (void)arg;
    task_calls++;

    if (requeue_limit > 0)
    {
        requeue_limit--;
        (void)seq_task_add(task_requeues, NULL);
    }
}

/*****************************************************************************************************/
/**
 * @brief A task that adds its argument to a running total.
 */
static void task_sums(void *arg)
{
    task_calls++;

    /* Same reason as state_counts_into_user: a wrong argument should show up
       as a total that does not add up, not as a crash. */
    if (arg != NULL)
    {
        task_total += *(const int *)arg;
    }
}

/*****************************************************************************************************/
/**
 * @brief A state that gives up after five seconds, the way a real one would.
 */
static void state_waits_then_times_out(seq_t *handle)
{
    if (seq_time(handle) > 5000U)
    {
        seq_next(handle, state_b, 0U);
    }
}

/*****************************************************************************************************/
/**
 * @brief Stands in for a higher priority interrupt arriving mid update.
 *
 * It only fires when interrupts are enabled, because that is the one thing the
 * hardware guarantees and the only thing the fix can rely on.
 */
void seq_test_hook(void)
{
    if ((queue_preempt_with != NULL) && (seq_test_primask == 0))
    {
        seq_task_fn_t pending = queue_preempt_with;

        queue_preempt_with = NULL;
        (void)seq_task_add(pending, NULL);
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
void seq_test_set_primask(uint32_t value)
{
    seq_test_primask = (int)value;

    if ((seq_test_primask == 0) && (queue_preempt_with != NULL))
    {
        seq_task_fn_t pending = queue_preempt_with;

        queue_preempt_with = NULL;
        (void)seq_task_add(pending, NULL);
    }
}

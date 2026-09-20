# Changelog

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-09-20

### Changed

- **Renamed from `fsm` to `sequencer`.** The old name promised a finite state
  machine, and this is not one: there is no declared set of states, no events,
  and no transition table. It is a non blocking sequencer for callbacks with
  delays, plus a task queue for interrupts, and the name now says so.
- Every public name changes with it, so code written against `fsm` will not
  compile until it is updated. The mapping is one for one:

  | Was | Is now |
  |---|---|
  | `fsm.h`, `fsm.c`, `fsm_config.h` | `seq.h`, `seq.c`, `seq_config.h` |
  | `fsm_t`, `fsm_fn_t`, `fsm_err_t` | `seq_t`, `seq_fn_t`, `seq_err_t` |
  | `fsm_init`, `fsm_loop`, `fsm_next`, `fsm_time` | `seq_init`, `seq_loop`, `seq_next`, `seq_time` |
  | `fsm_task_add` | `seq_task_add` |
  | `FSM_MAX_TASKS`, `FSM_ERR_NONE`, `FSM_ERR_FULL` | `SEQ_MAX_TASKS`, `SEQ_ERR_NONE`, `SEQ_ERR_FULL` |

  A find and replace of `fsm_` to `seq_` and `FSM_` to `SEQ_` covers all of it.

- Sources moved to `src/`, flat, with the configuration beside them.
- `seq.h` no longer includes `main.h`. Include it yourself if you relied on that.
- State functions take `seq_fn_t` instead of `const void (*)(void)`.
- Licence changed to Apache-2.0.

### Added

- `seq_stop()` and `seq_running()`, to halt a sequence and ask whether it is halted.
- `SEQ_ERR_INVALID`, returned by `seq_task_add()` when the task pointer is NULL, so a bad argument is no longer reported as a full queue.
- `seq_task_peak()` and `seq_task_flush()`, to size `SEQ_MAX_TASKS` by measurement and to drop queued work.
- Host unit tests, run with `python test/run_tests.py`.
- CMake build, and `install.py` to install the library into a project.

### Fixed

- `seq_time()` returned zero inside a running state, so every timeout built on it silently never fired.
- A queued task could be lost when two interrupts called `seq_task_add()` at the same moment. `SEQ_ERR_NONE` was returned for both.
- `seq_time()` now returns 0 while the sequence is stopped, rather than a number that keeps climbing for a state nothing is running.
- `SEQ_MAX_TASKS` below 2 is refused at compile time. One slot is always left free, so a smaller queue could never accept anything, and it failed silently.

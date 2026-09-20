# Changelog

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project follows [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [2.0.0] - 2026-09-20

### Added

- `fsm_stop()` and `fsm_running()`, to halt a machine and ask whether it is halted.
- `fsm_task_peak()` and `fsm_task_flush()`, to size `FSM_MAX_TASKS` by measurement and to drop queued work.
- Host unit tests, run with `python test/run_tests.py`.
- CMake build, and `install.py` to install the library into a project.

### Changed

- Sources moved to `inc/` and `src/`, and the configuration template to `template/`.
- `fsm.h` no longer includes `main.h`. Include it yourself if you relied on that.
- State functions take `fsm_fn_t` instead of `const void (*)(void)`. Existing calls compile unchanged.
- Licence changed to Apache-2.0.

### Fixed

- `fsm_time()` returned zero inside a running state, so every timeout built on it silently never fired.
- A queued task could be lost when two interrupts called `fsm_task_add()` at the same moment. `FSM_ERR_NONE` was returned for both.

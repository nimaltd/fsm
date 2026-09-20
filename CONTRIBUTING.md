# Contributing

Thanks for wanting to help. Bug reports, fixes and new features are all welcome.
There is no contributor agreement to sign: under section 5 of the Apache Licence
2.0, anything you submit for inclusion is covered by the project's own licence.

## Reporting a bug

Open an issue and include the STM32 family you are using, whether you are on HAL
or LL, your `fsm_config.h` settings, and the smallest piece of code that shows
the problem. A failing test is even better, see below.

## Making a change

1. Add or update a test in `test/test_fsm.c` that fails before your change and
   passes after it. If a change cannot be covered by a test, say why in the pull
   request.
2. Run the tests:

   ```bash
   python test/run_tests.py
   ```

3. Match the existing code style. The short version: 4 spaces and no tabs, Allman
   braces, `snake_case`, every file scope name prefixed with `fsm_`, a Doxygen
   block on every public function, section banners at 103 columns, and comments
   in plain 7-bit ASCII with no em dashes. A `.clang-format` in the project root
   handles the mechanical parts:

   ```bash
   clang-format -i src/fsm.c src/fsm.h
   ```

4. Keep the public header free of vendor headers. `src/fsm.h` includes only
   `<stdint.h>` and `fsm_config.h`, which is what lets the tests run on a PC.
   Anything from the HAL belongs in `src/fsm.c`.

## What CI checks

Every pull request runs two jobs, and both must be green:

- the full test suite on Ubuntu, built with `-Wall -Wextra -Wpedantic -Werror`
- a cross compile of `src/fsm.c` for Cortex-M4 with the same warning settings

Warnings are errors, so a build that warns will not merge.

## Questions

Open an issue, or reach me at nima.askari@gmail.com.

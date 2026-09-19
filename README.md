# 🌀 fsm

[![CI](https://github.com/nimaltd/fsm/actions/workflows/ci.yml/badge.svg)](https://github.com/nimaltd/fsm/actions/workflows/ci.yml)
[![Stars](https://img.shields.io/github/stars/NimaLTD/fsm?style=social)](https://github.com/nimaltd/fsm)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue)](LICENSE.md)

A small finite state machine with a built in task queue, written in C for STM32.

The point of this library is to get `HAL_Delay()` out of your main loop. States change after a delay without blocking, and interrupts hand their work to a queue instead of doing it inside the handler. Your main loop stays responsive and your ISRs stay short.

It is around 100 lines of actual code, it needs no RTOS, and it works on any STM32 family.

---

## ✨ What you get

- Non blocking state transitions with millisecond resolution
- A lock free task queue that is safe to use from an interrupt
- One task runs per loop, so a burst of interrupts cannot starve your state machine
- No dynamic memory, no RTOS, no dependencies beyond the HAL tick
- Unit tested on every commit

---

## 📁 Layout

```
inc/        fsm.h
src/        fsm.c
template/   fsm_config.h, the copy you make your own
test/       host unit tests, run on a PC
```

---

## ⚙️ Installing it

### The easy way

Download this repository into your STM32 project, then from the project root:

```bash
python fsm/install.py
```

It flattens the repository into a plain library folder, creates your `fsm_config.h`, and adds the library to your CMake, STM32CubeIDE, Keil or IAR project for you. Your project file is backed up first.

If you would rather not download anything first:

```bash
pip install https://github.com/nimaltd/stm32-installer/archive/refs/heads/main.zip
stm32-install fsm
```

Run it from the project root and it asks which folder to use. The first line is needed once, not once per library.

### Or copy the files in by hand

1. Copy `inc/fsm.h` into your project's `Core/Inc`
2. Copy `src/fsm.c` into your project's `Core/Src`
3. Copy `template/fsm_config.h` into `Core/Inc`

The config file sits in its own folder on purpose. Once you have copied it, that copy is yours, so pulling a new version of the library never overwrites your settings.

### Or use CMake

```cmake
add_subdirectory(fsm)
target_link_libraries(your_app PRIVATE nimaltd::fsm)

# fsm.c needs your fsm_config.h and your main.h, which normally live together.
target_include_directories(fsm PRIVATE ${CMAKE_SOURCE_DIR}/Core/Inc)
```

---

## 🔧 Configuration

Everything lives in your `fsm_config.h`:

```c
#define FSM_MAX_TASKS       16U
```

One slot is always kept free so a full queue can be told apart from an empty one, so `16` gives you room for 15 queued tasks. The minimum is 2.

---

## 🚀 Getting started

```c
#include "fsm.h"

fsm_t my_fsm;

void state_idle(void)
{
    if (something_happened())
    {
        fsm_next(&my_fsm, state_measure, 0);
    }
}

void state_measure(void)
{
    start_measurement();

    /* Come back in 200 ms, without blocking anything. */
    fsm_next(&my_fsm, state_report, 200);
}

void state_report(void)
{
    send_result();
    fsm_next(&my_fsm, state_idle, 0);
}

int main(void)
{
    /* ... HAL init ... */

    fsm_init(&my_fsm, state_idle);

    while (1)
    {
        fsm_loop(&my_fsm);
    }
}
```

### Handing work over from an interrupt

This is the part that keeps your ISRs honest. The handler queues a function and returns immediately, and the work itself runs later from the main loop.

```c
void button_pressed(void)
{
    /* Runs from fsm_loop(), so you can take your time here. */
    read_sensor();
    update_display();
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == B1_Pin)
    {
        fsm_task_add(button_pressed);
    }
}
```

---

## 🧰 API

| Function | What it does |
|---|---|
| `void fsm_init(fsm_t *handle, fsm_fn_t first_fn)` | Set up a handle and the state it starts from |
| `void fsm_loop(fsm_t *handle)` | Run queued tasks and the current state. Call it from your main loop |
| `void fsm_next(fsm_t *handle, fsm_fn_t next_fn, uint32_t delay_ms)` | Choose the next state, optionally after a delay |
| `uint32_t fsm_time(const fsm_t *handle)` | How long the machine has been in the current state |
| `fsm_err_t fsm_task_add(fsm_fn_t task_fn)` | Queue a task. Safe to call from an interrupt |

`fsm_task_add()` returns `FSM_ERR_NONE` when the task was queued, or `FSM_ERR_FULL` when the queue is full.

---

## 🧪 Running the tests

The tests run on your PC, not on hardware. Time is faked, so a 200 ms delay is tested instantly. You need cmake and any C compiler, nothing else: [Unity](https://github.com/ThrowTheSwitch/Unity) is vendored into `test/unity/`, so there is nothing to install.

One command does everything:

```bash
python test/run_tests.py
```

It configures, builds and runs the suite, then tells you plainly whether it passed. Add `--clean` to start from an empty build folder.

If you prefer doing it by hand:

```bash
cmake -S . -B build -DFSM_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## ⬆️ Coming from version 1

Nothing in your state functions needs to change, but three things moved:

- The files now live in `inc/` and `src/` instead of the repository root
- `fsm_config.h` now ships in `template/`. Copy it once and that copy is yours from then on
- `fsm.h` no longer includes `main.h`. If a file of yours relied on that, include `main.h` yourself

The function signatures now use `fsm_fn_t` instead of `const void (*)(void)`. Existing calls compile unchanged, and the old form produced a warning on some compilers, which this fixes.

---

## 🤝 Contributing

Bug reports and pull requests are welcome. See [CONTRIBUTING.md](CONTRIBUTING.md) for the style rules and how to run the tests. Nothing to sign, just open a pull request.

---

## 💖 Support

I write these libraries in my own time and give them away, because good tools should be easy to get. If this one saved you an afternoon, there are two things that genuinely help:

**⭐ Star the repo.** It costs you one click, it helps other engineers find the library, and it is the main reason I keep going.

**☕ [Buy me a coffee on Ko-fi](https://ko-fi.com/nimaltd).** Any amount is a real motivation to keep writing, documenting and maintaining this work.

[![GitHub](https://img.shields.io/badge/GitHub-Follow-black?style=for-the-badge&logo=github)](https://github.com/NimaLTD)
[![YouTube](https://img.shields.io/badge/YouTube-Subscribe-red?style=for-the-badge&logo=youtube)](https://youtube.com/@nimaltd)
[![Instagram](https://img.shields.io/badge/Instagram-Follow-purple?style=for-the-badge&logo=instagram)](https://instagram.com/github.nimaltd)
[![LinkedIn](https://img.shields.io/badge/LinkedIn-Connect-blue?style=for-the-badge&logo=linkedin)](https://linkedin.com/in/nimaltd)
[![Email](https://img.shields.io/badge/Email-Contact-red?style=for-the-badge&logo=gmail)](mailto:nima.askari@gmail.com)
[![Ko-fi](https://img.shields.io/badge/Ko--fi-Support-orange?style=for-the-badge&logo=ko-fi)](https://ko-fi.com/nimaltd)

---

## 📜 License

Apache License 2.0. See [LICENSE.md](LICENSE.md).

You are free to use this in commercial and closed source products. What the license asks in return is that you keep the copyright notice and pass along the [NOTICE](NOTICE) file, so the credit travels with the code.

The test folder vendors [Unity](https://github.com/ThrowTheSwitch/Unity) under its own MIT license, kept in [test/unity/LICENSE.txt](test/unity/LICENSE.txt). It is only used for testing and is not part of what you flash to a device.

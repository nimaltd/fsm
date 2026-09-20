# 🌀 sequencer

[![CI](https://github.com/nimaltd/sequencer/actions/workflows/ci.yml/badge.svg)](https://github.com/nimaltd/sequencer/actions/workflows/ci.yml)
[![Stars](https://img.shields.io/github/stars/NimaLTD/sequencer?style=social)](https://github.com/nimaltd/sequencer)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue)](LICENSE.md)

A small non blocking state sequencer with a built in task queue, written in C for STM32.

The point of this library is to get `HAL_Delay()` out of your main loop. States change after a delay without blocking, and interrupts hand their work to a queue instead of doing it inside the handler. Your main loop stays responsive and your ISRs stay short.

It is around 100 lines of actual code, it needs no RTOS, and it works on any STM32 family.

---

## ✨ What you get

- Non blocking state transitions with millisecond resolution
- A lock free task queue that is safe to use from an interrupt
- One task runs per loop, so a burst of interrupts cannot starve your sequence
- No dynamic memory, no RTOS, no dependencies beyond the HAL tick
- Unit tested on every commit

---

## 📁 Layout

```
src/    seq.h, seq.c, seq_config.h
test/   host unit tests, run on a PC
```

---

## ⚙️ Installing it

### The easy way

Download this repository into your STM32 project, then from the project root:

```bash
python sequencer/install.py
```

It flattens the repository into a plain library folder, creates your `seq_config.h`, and adds the library to your CMake, STM32CubeIDE, Keil or IAR project for you. Your project file is backed up first.

Nothing is installed on your machine and there is no pip step. The installer is fetched into a temporary folder, used, and deleted.

### Or install it without downloading the repository

Run this from the root of your STM32 project. One line, and it knows which library it belongs to because that is the repository it came from.

**Windows, Command Prompt:**

```bat
curl -fsSL https://raw.githubusercontent.com/nimaltd/sequencer/master/install.py -o install.py && python install.py
```

**Windows, PowerShell:**

```powershell
irm https://raw.githubusercontent.com/nimaltd/sequencer/master/install.py -OutFile install.py; python install.py
```

PowerShell needs `irm` here rather than `curl`, because in PowerShell `curl` is an alias for a different command that does not understand those options.

**Linux and macOS:**

```bash
curl -fsSL https://raw.githubusercontent.com/nimaltd/sequencer/master/install.py -o install.py && python3 install.py
```

It asks which folder to use, then downloads only the files the library actually needs, not the whole repository. Afterwards it deletes itself, so your project is left with the library and nothing else.

To update later, run the same one line again.

### Pinning a version

By default you get the newest code on `master`. To hold a project on one release instead, add `--ref` with a tag:

```bash
python install.py --ref 2.0.0
```

A branch name or a commit hash works there too, which is useful when you need exactly what you built with last time.

### Or copy the files in by hand

1. Copy `src/seq.h` into your project's `Core/Inc`
2. Copy `src/seq.c` into your project's `Core/Src`
3. Copy `src/seq_config.h` into `Core/Inc`

Once you have copied it, that copy is yours. The installer creates it only when it is missing, so updating the library never overwrites a setting you changed.

### Or add the whole repository to a CMake build

If you keep this repository as a submodule rather than installing it:

```cmake
add_subdirectory(sequencer)
target_link_libraries(your_app PRIVATE nimaltd::seq)

# This line is needed because the target above is a static library, which does
# not inherit your application's include paths, and seq.c has to find your
# seq_config.h and your main.h.
target_include_directories(seq PRIVATE ${CMAKE_SOURCE_DIR}/Core/Inc)
```

`python install.py` avoids that last line entirely: it writes an INTERFACE target instead, whose sources compile as part of your own target and inherit everything it has.

---

## 🔧 Configuration

Everything lives in your `seq_config.h`:

```c
#define SEQ_MAX_TASKS       16U
```

One slot is always kept free so a full queue can be told apart from an empty one, so `16` gives you room for 15 queued tasks. The minimum is 2.

---

## 🚀 Getting started

```c
#include "seq.h"

seq_t my_seq;

void state_idle(void)
{
    if (something_happened())
    {
        seq_next(&my_seq, state_measure, 0);
    }
}

void state_measure(void)
{
    start_measurement();

    /* Come back in 200 ms, without blocking anything. */
    seq_next(&my_seq, state_report, 200);
}

void state_report(void)
{
    send_result();
    seq_next(&my_seq, state_idle, 0);
}

int main(void)
{
    /* ... HAL init ... */

    seq_init(&my_seq, state_idle);

    while (1)
    {
        seq_loop(&my_seq);
    }
}
```

### Handing work over from an interrupt

This is the part that keeps your ISRs honest. The handler queues a function and returns immediately, and the work itself runs later from the main loop.

```c
void button_pressed(void)
{
    /* Runs from seq_loop(), so you can take your time here. */
    read_sensor();
    update_display();
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == B1_Pin)
    {
        seq_task_add(button_pressed);
    }
}
```

### What is safe to call from an interrupt

`seq_task_add()`, and nothing else.

Everything else expects to be called from the same place as `seq_loop()`, normally the main loop. `seq_next()` and `seq_stop()` each write several fields of the handle, and an interrupt landing in the middle leaves it half updated, most often with the new state set but not its delay, so it runs immediately instead of waiting. Nothing reports this, and it only happens on the timing where the interrupt lands badly, which is the worst kind of bug to chase.

So changing state from an interrupt looks like this, not like a direct call:

```c
static void on_button(void)
{
    seq_next(&my_seq, state_pressed, 500);   /* main loop, safe */
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    seq_task_add(on_button);                 /* the interrupt only hands over */
}
```

---

## 🧰 API

| Function | What it does |
|---|---|
| `void seq_init(seq_t *handle, seq_fn_t first_fn)` | Set up a handle and the state it starts from |
| `void seq_loop(seq_t *handle)` | Run queued tasks and the current state. Call it from your main loop |
| `void seq_next(seq_t *handle, seq_fn_t next_fn, uint32_t delay_ms)` | Choose the next state, optionally after a delay |
| `uint32_t seq_time(const seq_t *handle)` | How long the machine has been in the current state |
| `void seq_stop(seq_t *handle)` | Halt the machine. Nothing runs until the next `seq_next()` |
| `bool seq_running(const seq_t *handle)` | False once stopped |
| `seq_err_t seq_task_add(seq_fn_t task_fn)` | Queue a task. Safe to call from an interrupt |
| `uint32_t seq_task_peak(void)` | The most tasks ever queued at once |
| `void seq_task_flush(void)` | Drop everything queued |

`seq_task_add()` returns `SEQ_ERR_NONE` when the task was queued, or `SEQ_ERR_FULL` when the queue is full. It is the only call that is safe from an interrupt, and `seq_loop()` must have a single caller, since two would both consume the queue and could take the same task twice.

Use `seq_task_peak()` to size `SEQ_MAX_TASKS` by measurement. A full queue is reported to the caller, but that caller is usually an interrupt handler where nobody checks a return value, so the peak is in practice the only way to find out you were close to overflowing.

Stopping does not stop the task queue. Tasks belong to the application rather than to any one machine, so `seq_loop()` keeps serving them even on a stopped machine.

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
cmake -S . -B build -DSEQ_BUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

---

## ⬆️ Coming from version 1

Nothing in your state functions needs to change, but three things moved:

- The files now live in `src/` instead of the repository root
- `seq_config.h` now ships in `src/`. Copy it once and that copy is yours from then on
- `seq.h` no longer includes `main.h`. If a file of yours relied on that, include `main.h` yourself

The function signatures now use `seq_fn_t` instead of `const void (*)(void)`. Existing calls compile unchanged, and the old form produced a warning on some compilers, which this fixes.

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

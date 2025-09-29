# 🌀 Finite State Machine + Task Manager Library (FSM)

Lightweight, non-blocking FSM with built-in task queue support for embedded systems (e.g. STM32/HAL).

Designed to let you manage state transitions **without blocking delays**, while also scheduling small tasks to run “in-between” states.

---

## ✅ Supported Features / Use Cases

- Simple state machine control with delayed transitions  
- Task queue to schedule “background” tasks without blocking state processing  
- Works on any HAL-based platform (e.g. STM32)  
- Low overhead (pure C, minimal dependencies)  
- Useful in systems where you want to avoid `while(delay)` loops or blocking delay calls  

---

## ✨ Features

- **Non-blocking delays**: transition to the next state after a delay (ms)a without blocking execution  
- **Task queue**: schedule tasks (function pointers) to run before the next state  
- **Elapsed-time query**: query how long the current state has been active  
- **Configurable queue size**: via `fsm_config.h`  
- **Assertion-based parameter checks** (requires HAL / `assert_param`)  

---

## 🛠️ Installation

1. Copy `fsm.h`, `fsm_config.h`, and `fsm.c` into your project (e.g. into your `Drivers` or `Src` folder).  
2. Include `fsm.h` wherever you want to use the FSM.  
3. Ensure `fsm_config.h` is in your include path.  

---

## ⚙️ Configuration (`fsm_config.h`)

```c
/* USER CODE BEGIN FSM_INCLUDES */
/* USER CODE END FSM_INCLUDES */

/* USER CODE BEGIN FSM_CONFIGURATION */
#define FSM_MAX_TASKS 16
/* USER CODE END FSM_CONFIGURATION */
```

- **FSM_MAX_TASKS**: Maximum number of pending tasks in the queue.  
- You may also add includes or custom defines in the USER CODE regions.  

---

## 🧩 Integration / Setup (Platform Specific)

- Ensure that `HAL_GetTick()` is available (for timekeeping).  
- Include `assert_param(...)` or an equivalent assertion mechanism.  
- In your hardware setup, ensure the tick timer is running (e.g. SysTick) so HAL_GetTick() increments.

---

## 🚀 Quick Start / Example Usage

```c
#include "fsm.h"

static fsm_t my_fsm;

/* Forward declarations of state functions */
void state_idle(void);
void state_work(void);

/* In your init function */
fsm_init(&my_fsm, state_idle);

/* In your main loop */
while (1) {
    fsm_loop(&my_fsm);
    // other things you want to do
}

/* Definition of states */
void state_idle(void) {
    // do something in idle
    // then schedule next
    fsm_next(&my_fsm, state_work, 500);  // after 500 ms go to state_work
}

void state_work(void) {
    // do work
    // maybe add a background task
    fsm_task_add(&my_fsm, some_task_fn);
    // go back to idle or another state
    fsm_next(&my_fsm, state_idle, 1000);
}

void some_task_fn(void) {
    // small function to run between state transitions
}
```

- `fsm_loop(...)` should be called frequently (e.g. in main loop or periodic scheduler).  
- If there are pending tasks in the queue, they run *before* state transitions.  
- Delays are non-blocking: `fsm_next` just sets up the next state and delay, but does not block execution.

---

## 📋 API Reference

| Function | Description |
|----------|-------------|
| `fsm_init(fsm_t *handle, const void (*first_fn)(void))` | Initialize the FSM handle and set the first state |
| `fsm_loop(fsm_t *handle)` | Should be called periodically; executes tasks or advances state |
| `fsm_next(fsm_t *handle, const void (*next_fn)(void), uint32_t delay_ms)` | Schedule next state after a delay (ms) |
| `fsm_time(fsm_t *handle)` | Return elapsed milliseconds since entering the current state |
| `fsm_task_add(fsm_t *handle, const void (*new_task_fn)(void))` | Add a task to run before the next state; returns # define `FSM_ERR_NONE` or `FSM_ERR_FULL` |

---

## 📦 License & Attribution

This library is **dual-licensed**. Please see the `LICENSE` file for details.  
Copyright (C) 2025 Nima Askari – NimaLTD. All rights reserved.

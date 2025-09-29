# 🌀 Finite State Machine + Task Manager Library for STM32  

A lightweight and efficient **Finite State Machine (FSM)** library with built-in **non-blocking task manager**, written in C for STM32 (HAL-based).  

Unlike blocking implementations, this library uses **time-based state transitions** and a **task queue system** to keep your main loop responsive.  
It can be used in **any CubeMX STM32 project**, making it fully flexible and easy to integrate.  

It is designed for:  

- Projects where CPU blocking (`HAL_Delay`) must be avoided  
- Applications that need **clean state handling**  
- Event-driven systems that use **interrupts** (EXTI, UART, Timers)  
- Easy portability across STM32 families (F0/F1/F3/F4/F7/G0/G4/H7, etc.)  

---

## ✨ Features  

- 🔹 Non-blocking **state transitions** with millisecond resolution  
- 🔹 Built-in **task queue system** for background jobs  
- 🔹 Works with **interrupt callbacks** (e.g. EXTI, UART, Timer)  
- 🔹 Fully HAL-compatible (`HAL_GetTick()`)  
- 🔹 Small memory footprint, portable C code  
- 🔹 Simple, clean API  

---

## ⚙️ Installation  

You can install in two ways:  

### 1. Copy files directly  
Add these files to your STM32 project:  
- `fsm.h`  
- `fsm.c`  
- `fsm_config.h`  

### 2. STM32Cube Pack Installer (Recommended)  
Available in the official pack repo:  
👉 [STM32-PACK](https://github.com/nimaltd/STM32-PACK)  (Not Ready)  

---

## 🔧 Configuration (`fsm_config.h`)  

Defines library limits and task queue size. Example:  

```c
#define FSM_MAX_TASKS     16    // Max number of queued tasks
```  

---

## 🛠 CubeMX Setup  

1. **System Tick Timer**  
   - HAL must have a working SysTick (default in CubeMX).  

2. **GPIO (Optional)**  
   - Configure a button as **External Interrupt** (e.g., `B1` on `PC13`).  

3. **NVIC**  
   - Enable EXTI line interrupt in CubeMX for your button pin.  

---

## 🚀 Quick Start  

### Include header  
```c
#include "fsm.h"
```  

### Define handle  
```c
fsm_t hFsm;
```  

### Initialize in `main.c`  
```c
fsm_init(&hFsm, state_idle);

while (1)
{
    fsm_loop(&hFsm);   // must be called frequently
}
```  

### Example: Using FSM with EXTI Button Interrupt  

```c
/* ===== States ===== */
void state_idle(void)
{
    // stay idle until button event
    fsm_next(&hFsm, state_idle, 1000);  // re-check after 1s
}

void state_button(void)
{
    // toggle LED on button press
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);

    // go back to idle after 200 ms
    fsm_next(&hFsm, state_idle, 200);
}

/* ===== Task Function ===== */
void button_task(void)
{
    // switch FSM to button state
    fsm_next(&hFsm, state_button, 0);
}

/* ===== Interrupt Callback ===== */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == B1_Pin) // User button
    {
        // Schedule FSM task safely from interrupt
        fsm_task_add(&hFsm, button_task);
    }
}
```

✅ ISR stays short → only `fsm_task_add()` is called  
✅ FSM executes logic in main loop → no blocking in ISR  

---

## 🧰 API Overview  

| Function | Description |
|----------|-------------|
| `fsm_init()` | Initialize FSM handle with first state |
| `fsm_loop()` | Main loop handler; runs tasks and state changes |
| `fsm_next()` | Schedule next state after delay (ms) |
| `fsm_time()` | Get elapsed time in current state |
| `fsm_task_add()` | Add a function task to be executed by FSM |

---

## 💖 Support  

If you find this project useful, please **⭐ star** the repo and consider supporting!  

- [![GitHub](https://img.shields.io/badge/GitHub-Follow-black?style=for-the-badge&logo=github)](https://github.com/NimaLTD)  
- [![YouTube](https://img.shields.io/badge/YouTube-Subscribe-red?style=for-the-badge&logo=youtube)](https://youtube.com/@nimaltd)
- [![Instagram](https://img.shields.io/badge/Instagram-Follow-blue?style=for-the-badge&logo=instagram)](https://instagram.com/github.nimaltd)
- [![LinkedIn](https://img.shields.io/badge/LinkedIn-Connect-blue?style=for-the-badge&logo=linkedin)](https://linkedin.com/in/nimaltd)
- [![Email](https://img.shields.io/badge/Email-Contact-red?style=for-the-badge&logo=gmail)](mailto:nima.askari@gmail.com)
- [![Ko-fi](https://img.shields.io/badge/Ko--fi-Support-orange?style=for-the-badge&logo=ko-fi)](https://ko-fi.com/nimaltd)

---

## 📜 License  

Licensed under the terms in the [LICENSE](./LICENSE.TXT).  

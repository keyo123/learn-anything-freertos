# FreeRTOS 软件定时器 (Software Timers)

## 🎯 目标
- 掌握如何使用 `xTimerCreate()` 创建单次（One-Shot）和周期性（Auto-Reload）软件定时器。
- 掌握定时器启动（`xTimerStart`）、复位（`xTimerReset`）以及动态改变周期（`xTimerChangePeriod`）的 API。
- 深刻理解软件定时器回调函数的**不准阻塞**规则及背后的 Timer Daemon 任务工作机制。

---

## 📋 背景
FreeRTOS 的软件定时器是基于任务（Daemon Task）和命令队列实现的，并不直接运行在硬件中断中。
* **周期定时器（Auto-Reload）**：在超时后自动重载，周期性执行回调函数。适合系统心跳、状态轮询等周期性任务。
* **单次定时器（One-Shot）**：超时并执行回调后进入休眠状态，不会自动重启。适合延时关闭背光、超时重连等单次延时场景。

⚠️ **黄金法则：软件定时器回调函数中绝对不能调用任何可能引起阻塞的 API（如 `vTaskDelay`，或不带 0 超时的 `xQueueReceive`/`xSemaphoreTake`），否则会饿死整个软件定时器队列，导致所有定时器瘫痪！**

---

## ✅ 练习要求

### 练习 10.1 — 周期心跳定时器 (Auto-Reload)
- 创建一个名为 `"HeartbeatTimer"` 的周期定时器，周期为 1000ms（1秒）。
- 在回调函数中，翻转开发板上的 LED1，并打印日志 `[Heartbeat] Tick! LED1 Toggled.`。

### 练习 10.2 — 背光自动关闭与重置 (One-Shot & Reset)
- 创建一个名为 `"BacklightTimer"` 的单次定时器，周期为 5000ms（5秒）。
- 超时回调函数负责关闭背光（打印 `[Backlight] Timeout! Screen Backlight OFF.`）。
- 编写一个模拟按键触发屏幕活跃的任务 `vTaskBacklightController`：
  - 前 3 秒，每 1.5 秒打印一次模拟按键事件，并调用 `xTimerReset` 刷新背光定时器。
  - 观察定时器是否被成功重置并推迟了关闭时间（总延迟应当大于 5 秒）。
  - 停止按键后，等待 5 秒，观察背光定时器正常超时并自动关闭。

### 练习 10.3 — 动态心跳速率调整 (Change Period)
- 在背光控制器中，前 3 秒除了模拟按键，还要将心跳定时器的周期由 1000ms 动态缩短为 200ms。
- 背光关闭后，再将心跳定时器的周期恢复为 1000ms。
- 观察心跳日志的输出频率是否发生动态变化。

---

## 🔧 需要修改/创建的文件
- `.learn/topics/freertos/exercises/software-timer/starter.c` — 练习模板
- `01-FreeRTOS-LED/User/demo_timer.c` — 实际运行的演示代码
- `01-FreeRTOS-LED/User/demos.h` — 声明新演示入口
- `01-FreeRTOS-LED/User/main.c` — 启动软件定时器演示

# 中断安全 API (Interrupt Safe APIs)

## 🎯 目标
理解并验证 FreeRTOS 中的中断安全 API (`FromISR` 系列函数) 的运行机制，深入掌握 `xHigherPriorityTaskWoken` 参数与 `portYIELD_FROM_ISR()` 对系统实时响应的决定性作用。

## 📋 背景
在 FreeRTOS 中，普通的 API 函数（如 `xSemaphoreGive()`）不能在中断服务程序 (ISR) 中使用。必须使用带 `FromISR` 后缀的函数（如 `xSemaphoreGiveFromISR()`）。
此外，这些函数带有一个特殊的参数 `pxHigherPriorityTaskWoken`。当该函数唤醒了一个优先级高于或等于当前运行任务的任务时，该指针会被写入 `pdTRUE`。我们在退出中断前，必须通过 `portYIELD_FROM_ISR()` 手动触发一次上下文切换，以实现即时调度。

## ✅ 练习要求

### 1. 编写中断同步任务
- 在 `demo_isr.c` 中，定义一个二值信号量 `xISRTestSem`。
- 创建一个高优先级任务 `vTaskISRHandler`（优先级设为 3），它等待该信号量，并在获取后翻转 LED1 并打印被唤醒的时间戳。

### 2. 模拟中断触发同步
- 为了方便观察且不受按键抖动影响，在 `demo_isr.c` 中创建一个中等优先级的定时器/软件触发源任务 `vTaskTrigger`（优先级为 2，低于 Handler）。它每隔 2 秒通过触发一个挂起的软件中断，或者为了简单起见，我们直接在串口输入/或软件中模拟一个中断的触发。
- *更直接地*，我们可以修改 `stm32f10x_it.c` 中的按键中断处理，让它向 `xISRTestSem` 发送信号。

### 3. 对比实验：是否调用 `portYIELD_FROM_ISR`
在中断处理程序中，进行如下对比：
* **实验 A**：调用 `xSemaphoreGiveFromISR` 时，不传入 `xHigherPriorityTaskWoken` 的状态变化（或者传入后不调用 `portYIELD_FROM_ISR`）。
  - **观察**：按键按下后，LED1 的翻转和串口打印是否有明显的延迟（通常会延迟到下一个系统 Tick 中断，即最多延迟 1ms）。
* **实验 B**：正确保存 `xHigherPriorityTaskWoken` 并在退出中断前执行 `portYIELD_FROM_ISR(xHigherPriorityTaskWoken)`。
  - **观察**：按键按下后，高优先级任务是否能够在中断结束的瞬间**立刻抢占**并执行，响应延迟接近于 0。

---

## 🔧 需要修改的文件
- `01-FreeRTOS-LED/User/demo_isr.c` (我们将为你创建，并同步更新 `eide.yml` 项目配置)
- `01-FreeRTOS-LED/User/demos.h` (加入 `start_isr_demo` 声明)
- `01-FreeRTOS-LED/User/main.c` (在 `AppTaskCreate` 中调用 `start_isr_demo`)
- `01-FreeRTOS-LED/User/stm32f10x_it.c` (配置按键中断的同步逻辑)

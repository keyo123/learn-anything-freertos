# FreeRTOS 互斥量 (Mutex) 练习

## 🎯 目标
通过编写代码理解 FreeRTOS 互斥量的核心特性：资源保护、优先级继承，以及与二值信号量的关键区别。

## 📋 背景
你正在设计一个多任务系统，两个任务需要访问同一个共享资源（例如一个 LCD 显示屏或共享数据结构）。如果不加保护，任务切换可能导致数据竞争。你需要选择正确的同步原语。

## ✅ 要求

### 练习 1：保护共享资源
- [ ] 创建一个互斥量
- [ ] 创建两个任务，它们共享一个全局变量（如 `shared_counter`）
- [ ] 每个任务在访问共享变量前先 `Take` 互斥量，访问完后 `Give`
- [ ] 每个任务对共享变量做多次递增操作（例如循环 5 次，每次加 1）
- [ ] 观察并验证最终结果是否等于 `task_count * loop_count`

### 练习 2：理解优先级继承
- [ ] 创建三个任务：低优先级(L)、中优先级(M)、高优先级(H)
- [ ] L 先获取互斥量，然后被 H 抢占
- [ ] H 尝试获取同一互斥量时阻塞
- [ ] 观察 L 的优先级是否被临时提升到 H 的级别（优先级继承）
- [ ] 完成后将互斥量替换为二值信号量，观察行为差异

### 练习 3：验证约束
- [ ] 尝试在 ISR 中调用 `xSemaphoreGiveFromISR()` 给互斥量——观察结果
- [ ] 尝试让任务 A Take 互斥量后，任务 B 去 Give——观察结果

## 💡 提示

<details>
<summary>Hint 1: 互斥量创建</summary>
使用 `xSemaphoreCreateMutex()` 创建，不需要指定初始值（与二值信号量不同）。
返回 `SemaphoreHandle_t` 类型。
</details>

<details>
<summary>Hint 2: 优先级继承验证方法</summary>
在低优先级任务获取互斥量前后分别打印 `uxTaskPriorityGet()` 的值。
</details>

<details>
<summary>Hint 3: 二值信号量与 Mutex 的关键区别</summary>
- Mutex 有优先级继承，二值信号量没有
- Mutex 必须由同一个任务 Take 和 Give，二值信号量可以跨任务
- Mutex 不能在 ISR 中使用，二值信号量可以（用 GiveFromISR/TakeFromISR）
- Mutex 创建后初始值为 1（已释放），二值信号量初始值为 0
</details>

## 📎 相关概念
- 二值信号量 — 理解它与 Mutex 的异同
- 任务优先级 — 优先级继承机制的基础
- 调度机制 — 抢占式调度下 Mutex 的行为

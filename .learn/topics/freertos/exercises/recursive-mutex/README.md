# 递归互斥量 (Recursive Mutex)

## 🎯 目标
编写并验证**递归互斥量**在多层嵌套调用和递归函数中的同步行为，通过对比实验亲身体会它与普通互斥量的核心区别（防止自我锁死、嵌套计数逻辑）。

## 📋 背景
普通的互斥量（Mutex）是**不具备重入性**的。如果一个任务在已经持有了某个普通 Mutex 的情况下，再次尝试 `Take` 该 Mutex，任务会因为等待“自己释放该锁”而陷入永久的死锁状态。
为了解决这一问题，FreeRTOS 提供了**递归互斥量**（`Recursive Mutex`）。它会记录当前的“锁拥有者”以及“嵌套获取深度”。同一个任务可以无限次成功获取它，但必须释放相同的次数，锁才会真正重新可用。

## ✅ 练习要求

### 1. 递归嵌套调用
- 在 `demo_rec_mutex.c` 中，使用 `xSemaphoreCreateRecursiveMutex()` 创建一个递归互斥量 `xRecMutex`。
- 编写任务 `vTaskRecursiveDemo`，该任务会调用一个递归函数 `print_nested_log(depth)`。该函数内部首先 `xSemaphoreTakeRecursive` 获取锁，然后递归调用自身，最后 `xSemaphoreGiveRecursive` 释放锁。
- 观察并验证任务是否能正常递归执行，没有发生死锁。

### 2. 对比实验 A：换成普通 Mutex 观察死锁
- 在 `start_rec_mutex_demo` 中，将 `xRecMutex` 改为使用 `xSemaphoreCreateMutex()` 创建的普通互斥量。
- 同时在任务中使用 `xSemaphoreTake` 和 `xSemaphoreGive` 代替递归 API。
- **观察**：程序运行到第几层嵌套时会突然卡死？（提示：你应该会看到它在第一次进入后，第二次尝试拿锁时彻底挂起）。

### 3. 对比实验 B：成对释放的重要性
- 恢复为递归互斥量。
- 在 `vTaskRecursiveDemo` 中，故意制造“只 Take 不 Give 完”的逻辑错误（例如：Take 了 3 次，但只 Give 了 2 次）。
- 创建另一个低优先级的任务 `vTaskObserver`，让它尝试获取同一个递归互斥量。
- **观察**：由于计数器未归零，`vTaskObserver` 是否能获取到该锁？

---

## 🔧 需要修改的文件
- `01-FreeRTOS-LED/User/demo_rec_mutex.c` (我们将为你创建，并同步更新 `eide.yml` 项目配置)
- `01-FreeRTOS-LED/User/demos.h` (加入 `start_rec_mutex_demo` 声明)
- `01-FreeRTOS-LED/User/main.c` (在 `AppTaskCreate` 中调用 `start_rec_mutex_demo`)

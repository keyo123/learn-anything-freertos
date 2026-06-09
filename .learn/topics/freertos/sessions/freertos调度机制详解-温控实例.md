# FreeRTOS 调度机制详解 —— 温控系统实例

## 1. 场景定义

设计一个温控系统，三个任务：

| 任务 | 周期 | 功能 |
|------|------|------|
| 传感器（S） | 10ms | 读取温度传感器 |
| 屏幕（D） | 100ms | 刷新 OLED 显示 |
| SD 卡（L） | 1000ms | 记录数据到 SD 卡 |

优先级相同，使用 `vTaskDelayUntil` 实现精确定时。

---

## 2. 代码

```c
void vSensorTask(void *pvParams) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1) {
        read_sensor();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(10));
    }
}

void vDisplayTask(void *pvParams) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1) {
        update_display();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(100));
    }
}

void vSDCardTask(void *pvParams) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    while(1) {
        log_to_sd();
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(1000));
    }
}

int main() {
    xTaskCreate(vSensorTask,  "Sensor",  configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vDisplayTask, "Display", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vSDCardTask,  "SDCard",  configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    vTaskStartScheduler();  // 启动调度器，不再返回
    return 0;
}
```

---

## 3. 逐帧执行流程

SysTick 每 1ms 中断一次，调度器借此做决策。

### 第一阶段：任务启动

```
t=0      创建三个任务，全部进入就绪态
          ↓
         调度器选一个：传感器（先创建先运行）
         传感器开始执行 (运行态)

t=0~9ms  传感器运行中...
         屏幕和 SD 卡在阻塞态
```

### 第二阶段：传感器周期性运行

```
t=10ms   传感器调用 vTaskDelayUntil，系统计算下次唤醒 = t=20ms
          ↓
         传感器进入阻塞态
          ↓
         检查就绪列表：
           - 屏幕 (t=100 > 10) → 阻塞
           - SD卡 (t=1000 > 10) → 阻塞
          ↓
         无就绪任务 → 运行空闲任务（或进入低功耗）

t=11~19ms  空闲态

t=20ms    SysTick 中断
          ↓
          检查各任务阻塞到期时间：
           - 传感器：20 == 20 → 就绪
           - 屏幕：  100 > 20 → 继续阻塞
           - SD卡： 1000 > 20 → 继续阻塞
          ↓
          传感器开始执行
```

### 第三阶段：竞争出现（t=100ms）

```
t=100ms    SysTick 中断
           ↓
           传感器：100 == 100 → 就绪
           屏幕：  100 == 100 → 就绪    （← 同时就绪！）
           SD卡：  1000 > 100 → 继续阻塞  （SD卡不参与本次调度！）
           ↓
           调度器决策：优先级相同（同级）
           ↓
           时间片轮转

t=100ms   运行传感器（第 1 个 tick）

t=101ms   SysTick → 传感器时间片到
           ↓
           抢占，暂停传感器，保存上下文
           切换给屏幕运行（第 1 个 tick）

t=102ms   屏幕时间片到
           ↓
           回到传感器继续运行
```

### 第四阶段：1 秒时刻 SD 卡加入

```
t=1000ms   SysTick 中断
           ↓
           传感器：1000 == 1000 → 就绪
           屏幕：  1000 == 1000 → 就绪
           SD卡：  1000 == 1000 → 就绪    （← 新加入竞争！）
           ↓
           三个同级任务轮转，每个最多跑 1 个 tick
```

---

## 4. 完整调度时间线图

```
任务    0   10  20  30   ...   100 101 102 103 ...  1000 1001 1002 1003
传感器 ███ ███ ███ ███   ...   ██░ ░░░ ███ ███  ...  ██░ ░░░ ░░░ ███
屏幕   ░░░ ░░░ ░░░ ░░░   ...   ░░░ ██░ ░░░ ░░░  ...  ░░░ ██░ ░░░ ░░░
SD卡   ░░░ ░░░ ░░░ ░░░   ...   ░░░ ░░░ ░░░ ░░░  ...  ░░░ ░░░ ██░ ░░░

██ = 运行    ░░ = 阻塞

注意：
 - t=100~102: 传感器和屏幕轮流运行（同级轮转），SD卡不参与
 - t=1000~1002: 三个任务轮流运行
 - SD 卡在 t=1000ms 之前全程不参与调度
```

---

## 5. 调度决策逻辑总结

```
SysTick 中断
    │
    ├── 遍历所有任务，检查唤醒时间
    │   ├── 唤醒时间到达 → 阻塞态 → 就绪态
    │   └── 唤醒时间未到 → 保持阻塞
    │
    ├── 就绪列表为空？
    │   ├── 是 → 运行空闲任务 / 进低功耗
    │   └── 否 → 进入调度决策
    │
    ├── 有更高优先级任务就绪？
    │   ├── 是 → 立即抢占，切换到高优先级任务
    │   └── 否 → 同级任务时间片轮转
    │
    └── 切换任务（保存当前上下文 → 恢复目标上下文）
```

---

## 6. 关键词对照

| 概念 | 说明 |
|------|------|
| SysTick | 硬件定时器中断，调度的"心跳"，通常 1ms |
| 抢占式调度 | 高优先级任务就绪时，立即中断低优先级任务 |
| 时间片轮转 | 同级任务轮流执行，每个任务 1 个 tick |
| vTaskDelayUntil | 精确周期延时，用于固定频率任务 |
| 空闲任务 | 所有任务都阻塞时运行，可在此进入低功耗 |

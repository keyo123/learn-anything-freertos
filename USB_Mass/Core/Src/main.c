/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dma.h"
#include "fatfs.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "usb_device.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "w25qxx.h"
#include "lvgl_ui.h"
#include "json_app.h"
#include "hal/lv_hal_tick.h"
#include "usbd_cdc_if.h"
#include <string.h>
#include <stdio.h>

/* FreeRTOS 头文件 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
/* 文件服务 */
#include "app_file_service.h"
/* LVGL 和 LCD 函数声明 */
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include "lcd_init.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
uint8_t file_res;
uint8_t flag = 0;
uint8_t file_res_1;
uint8_t file_res_2;
uint8_t file_res_3;
FILINFO FileInfo = {0};
DIR DirInfo;
DIR User_DirInfo;
FATFS fs; /* Work area (file system object) for logical drives */
FRESULT Res;
UINT br, bw; /* File R/W count */
BYTE work[_MAX_SS];
uint8_t Ctrl_CDC=0;

/* FreeRTOS 任务句柄 */
static TaskHandle_t lvgl_task_handle = NULL;
TaskHandle_t g_cdc_tx_task_handle = NULL;  /* 全局，用于 ISR 通知 */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void LvglTask(void *argument);
static void CdcTxTask(void *argument);
static void LedTask(void *argument);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#if _USE_LFN
WCHAR ff_convert(WCHAR chr, UINT dir)
{
    if (dir == 0) {
        // 多字节(ASCII) → UTF-16：直接返回（ASCII编码值=UTF-16低字节）
        return (chr <= 0x7F) ? chr : 0;
    } else {
        // UTF-16 → 多字节(ASCII)：仅返回低字节（过滤非ASCII字符）
        return (chr <= 0x7F) ? chr : 0;
    }
}

WCHAR ff_wtoupper (WCHAR chr)
{
    // 判断是否为小写英文字母（UTF-16中a-z的编码为0x0061~0x007A）
    if (chr >= 0x0061 && chr <= 0x007A) {
        return chr - 0x0020;  // 转为大写（A-Z：0x0041~0x005A）
    }
    // 其他字符（数字、符号、中文等）不转换，原样返回
    return chr;
}
#endif
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_SPI2_Init();
    MX_UART4_Init();
    MX_FATFS_Init();
    MX_TIM3_Init();
    HAL_TIM_Base_Start_IT(&htim3);  /* 必须先启动 TIM3，HAL_Delay 依赖它 */
    MX_USB_DEVICE_Init();
    MX_SPI1_Init();
    /* USER CODE BEGIN 2 */

    SPI_FLASH_CS_HIGH();
    W25QXX_TYPE = SPI_FLASH_ReadID();

    file_res = f_mount(&fs, "0:", 1);
    if (FR_NO_FILESYSTEM == file_res)
    {
        flag = 1;
        file_res_1 = f_mkfs("0:", 0, sizeof(work));
        file_res_2 = f_mount(NULL, "0:", 1);
        file_res_3 = f_mount(&fs, "0:", 1);
    }
    HAL_GPIO_WritePin(USB_ENABLE_GPIO_Port, USB_ENABLE_Pin, GPIO_PIN_RESET);
    HAL_Delay(50);  /* 等待主机检测到 USB D+ 上拉 */

    LCD_Init();
    lv_init();            // LVGL 初始化
    lv_port_disp_init();  // 注册LVGL的显示任务
    lv_port_indev_init(); // 按键注册

    create_test_ui();

    if (f_opendir(&DirInfo, (const TCHAR *)"0:") == FR_OK) /* 打开文件夹目录成功，目录信息已经在dir结构体中保存 */
        f_readdir(&DirInfo, &FileInfo); /* 读文件信息到文件状态结构体中 */

//    test_json_parser(); // 测试 json_parser

    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    /* CDC 接收计数信号量（用于 ISR→Task 数据传递）*/
    CDC_Init();

    /* 初始化文件服务任务（优先级 1，用于 FATFS 操作）*/
    FS_Init();

    /* 学习点：FreeRTOS 启动流程
     *
     * 1. xTaskCreate() — 创建任务
     *    参数：任务函数，任务名，栈深度(words)，参数，优先级，任务句柄
     *    LVGL 任务优先级设为 2（空闲任务=0，定时器服务任务=4）
     *
     * 2. vTaskStartScheduler() — 启动 FreeRTOS 调度器
     *    此函数会创建空闲任务和定时器服务任务（如果启用），
     *    然后通过 SVC 异常启动第一个任务。
     *    调用后不会再返回！
     */
    xTaskCreate(
        LvglTask,
        "LVGL",
        440,
        NULL,
        2,
        &lvgl_task_handle
    );

    /* 学习点：CDC TX 任务使用任务通知替代轮询
     * 优先级 3 > LVGL(2)，确保 USB 回传低延迟
     * 当无数据时阻塞在 ulTaskNotifyTake，不消耗 CPU */
    xTaskCreate(
        CdcTxTask,
        "CDC_TX",
        256,
        NULL,
        3,
        &g_cdc_tx_task_handle
    );

    xTaskCreate(
        LedTask,
        "LED",
        80,
        NULL,
        1,
        NULL
    );

    vTaskStartScheduler();  /* 启动调度器，不会返回 */

    /* 如果执行到这里说明 FreeRTOS 堆内存不足 */
    Error_Handler();
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL_DIV1_5;  /* 72MHz / 1.5 = 48MHz */
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/**
  * @brief LVGL 任务函数
  *
  * 学习点：这是一个 FreeRTOS 任务函数，具有以下特征：
  * - 无限循环，永不返回
  * - vTaskDelay() 让任务进入阻塞态，释放 CPU 给其他任务或空闲任务
  * - 对比原来 main loop 的 systemTick 轮询方式，vTaskDelay 更高效
  *   （阻塞期间 CPU 可以执行其他任务或进入低功耗）
  */
static void LvglTask(void *argument)
{
    (void)argument;

    while (1)
    {
        lv_timer_handler();   /* 处理 LVGL 内部定时任务（渲染、动画等）*/
        vTaskDelay(pdMS_TO_TICKS(5));  /* 阻塞 5ms，让出 CPU */
    }
}

/**
  * @brief LED 闪烁任务
  *        优先级 1，每 500ms 翻转一次 LED1(PB4)
  */
static void LedTask(void *argument)
{
    (void)argument;

    while (1)
    {
        // TODO: 在这里翻转 LED1（PB4）
        // 提示：HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
  * @brief CDC TX 任务函数
  *
  * 学习点：使用 FreeRTOS 任务通知替代固定间隔轮询。
  * ulTaskNotifyTake() 使任务阻塞，零 CPU 消耗；
  * USB ISR 通过 vTaskNotifyGiveFromISR() 唤醒任务，
  * 实现数据到达 → 立即发送，无需等待下一个轮询周期。
  *
  * 优先级 3（高于 LVGL 的 2），确保 USB 回传低延迟。
  */
static void CdcTxTask(void *argument)
{
    (void)argument;

    while (1)
    {
        /* 阻塞等待计数信号量 — 有新数据到达时 ISR 会递增计数值 */
        xSemaphoreTake(g_rx_sem, portMAX_DELAY);

        CDC_ProcessTx();
    }
}

/**
  * @brief TIM3 周期中断回调 — 为 LVGL 提供 1ms tick
  * HAL 时间基准由 SysTick 提供
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == htim3.Instance)
    {
        lv_tick_inc(1);
    }
}

/* SPI 传输完成回调函数 */
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance == SPI1)
    {
        /* 传输完成，可以在这里处理接收到的数据 */
    }
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

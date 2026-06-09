/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_cdc_if.c
  * @brief          : USB CDC Interface layer
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_cdc_if.h"

/* USER CODE BEGIN INCLUDE */
#include "usbd_composite.h"
#include "usbd_core.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <stdio.h>
#include <string.h>
#include "app_file_service.h"
/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
/* Cache of CDC handle to avoid pClassData dependency in callbacks */
static USBD_CDC_HandleTypeDef *g_hcdc = NULL;

/* ── 环形缓冲区 + 计数信号量（替代原来的单缓冲区方案）── */

#define CDC_RING_SIZE  16  /* 最多缓冲 16 个数据包 */

/* 环形缓冲区槽：每槽存一包数据 */
typedef struct {
    uint8_t  data[CDC_FS_MAX_PACKET_SIZE];
    uint16_t len;
} cdc_ring_slot_t;

static cdc_ring_slot_t g_rx_ring[CDC_RING_SIZE];
static volatile uint16_t g_rx_ring_wr = 0;  /* ISR 写入位置 */
static volatile uint16_t g_rx_ring_rd = 0;  /* Task 读取位置 */

/* 计数信号量 — 计数值 = 环形缓冲区中待发送的数据包数量
 * 初始值 0，最大值 CDC_RING_SIZE - 1（留一格判满）*/
SemaphoreHandle_t g_rx_sem = NULL;
/* USER CODE END PV */

/** @addtogroup STM32_USB_OTG_DEVICE_LIBRARY
  * @{
  */

/** @defgroup USBD_CDC_IF
  * @{
  */

/** @defgroup USBD_CDC_IF_Private_Defines
  * @{
  */
/* USER CODE BEGIN PRIVATE_DEFINES */
/* Define size for the receive and transmit buffer over CDC */
#define APP_RX_DATA_SIZE  2048
#define APP_TX_DATA_SIZE  2048
/* USER CODE END PRIVATE_DEFINES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_FunctionsPrototypes
  * @{
  */

/* USER CODE BEGIN PRIVATE_FUNCTIONS_DECLARATION */

/* USER CODE END PRIVATE_FUNCTIONS_DECLARATION */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Variables
  * @{
  */
/* USER CODE BEGIN PRIVATE_VARIABLES */
static volatile uint8_t g_cdc_connected = 0; /* PC 已打开 CDC 虚拟串口（DTR 信号） */
/* USER CODE END PRIVATE_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Exported_Variables
  * @{
  */
extern USBD_HandleTypeDef hUsbDeviceFS;

uint8_t UserRxBufferFS[CDC_FS_MAX_PACKET_SIZE];
uint8_t UserTxBufferFS[CDC_FS_MAX_PACKET_SIZE];

/* CDC Interface callback structure */
USBD_CDC_ItfTypeDef USBD_CDC_fops_FS =
{
  CDC_Control_FS,
  CDC_DataTx_FS,
  CDC_DataRx_FS
};

/* USER CODE BEGIN EXPORTED_VARIABLES */

/* USER CODE END EXPORTED_VARIABLES */

/**
  * @}
  */

/** @defgroup USBD_CDC_IF_Private_Functions
  * @{
  */

/**
  * @brief  CDC_Control_FS
  *         Manage CDC control requests.
  * @param  cmd: Control command
  * @param  pbuf: Pointer to buffer of data
  * @param  length: Length of data to be processed
  * @retval Result
  */
int8_t CDC_Control_FS(uint8_t cmd, uint8_t *pbuf, uint16_t length)
{
  /* USER CODE BEGIN 9 */
  switch (cmd)
  {
    case CDC_SET_LINE_CODING:
      break;

    case CDC_GET_LINE_CODING:
      break;

    case CDC_SET_CONTROL_LINE_STATE:
      /* pbuf 指向 SET_CONTROL_LINE_STATE 请求的 wValue（2字节），bit 0 = DTR */
      if (length >= 2) {
        g_cdc_connected = (pbuf[0] & 0x01) ? 1 : 0;
      }
      break;

    default:
      break;
  }

  return (USBD_OK);
  /* USER CODE END 9 */
}

/**
  * @brief  CDC_DataTx_FS
  *         Data transmitted callback.
  * @retval Result
  */
int8_t CDC_DataTx_FS(void)
{
  /* USER CODE BEGIN 11 */
  return (USBD_OK);
  /* USER CODE END 11 */
}

/**
  * @brief  CDC_DataRx_FS
  *         Data received callback.
  * @param  pbuf: Pointer to data buffer
  * @param  len: Pointer to data length
  * @retval Result
  */
int8_t CDC_DataRx_FS(uint8_t *pbuf, uint32_t *len)
{
  /* USER CODE BEGIN 13 */

  if (*len > 0)
  {
    uint16_t copylen = (uint16_t)(*len);
    if (copylen > CDC_FS_MAX_PACKET_SIZE)
    {
      copylen = CDC_FS_MAX_PACKET_SIZE;
    }

    /* 环形缓冲区的写入指针如果追上读取指针 → 缓冲区满，丢弃最老的数据 */
    uint16_t next = (g_rx_ring_wr + 1) % CDC_RING_SIZE;
    if (next != g_rx_ring_rd)
    {
      memcpy(g_rx_ring[g_rx_ring_wr].data, pbuf, copylen);
      g_rx_ring[g_rx_ring_wr].len = copylen;
      g_rx_ring_wr = next;

      /* 递增计数信号量 — 通知 CDC_Tx 任务有数据可用 */
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      xSemaphoreGiveFromISR(g_rx_sem, &xHigherPriorityTaskWoken);
      portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
    /* 如果缓冲区满，静默丢弃新数据（也可选择覆盖最老数据）*/
  }
  return (USBD_OK);
  /* USER CODE END 13 */
}

/**
  * @brief  CDC_CacheHandle
  *         Cache CDC handle pointer for use in callbacks.
  *         Must be called AFTER USBD_Start() when pClassData is valid.
  */
void CDC_CacheHandle(void)
{
  USBD_Composite_HandleTypeDef *hcomp;

  hcomp = (USBD_Composite_HandleTypeDef *)hUsbDeviceFS.pClassData;
  if (hcomp != NULL)
  {
    g_hcdc = &hcomp->cdc;
  }
}

/**
  * @brief  CDC_CacheHandleDirect
  *         Cache CDC handle from a known-good pointer.
  *         Called from Composite_Init with the correct hcomp address.
  * @param  hcdc: pointer to CDC sub-handle within composite handle
  */
void CDC_CacheHandleDirect(void *hcdc)
{
  g_hcdc = (USBD_CDC_HandleTypeDef *)hcdc;
}

/**
  * @brief  CDC_Transmit_FS
  *         Data to be sent over USB CDC.
  * @param  Buf: Pointer to data buffer
  * @param  Len: Data length
  * @retval Result
  */
int8_t CDC_Transmit_FS(uint8_t *Buf, uint16_t Len)
{
  uint8_t result = USBD_OK;
  /* USER CODE BEGIN 15 */
  if (g_hcdc == NULL)
  {
    return USBD_FAIL;
  }

  if (g_hcdc->TxState == 0U)
  {
    /* Prepare tx buffer for transmission */
    if (Len > CDC_FS_MAX_PACKET_SIZE)
    {
      Len = CDC_FS_MAX_PACKET_SIZE;
    }
    memcpy((uint8_t *)g_hcdc->data, Buf, Len);
    g_hcdc->RxLength = Len;

    /* USBD_CDC_TransmitPacket reads pClassData for TxState/data,
       but in main-loop context pClassData is the composite handle.
       Swap to CDC sub-handle so TxState/RxLength/data resolve correctly. */
    {
      void *saved = hUsbDeviceFS.pClassData;
      hUsbDeviceFS.pClassData = g_hcdc;
      result = USBD_CDC_TransmitPacket(&hUsbDeviceFS);
      hUsbDeviceFS.pClassData = saved;
    }
  }
  else
  {
    result = USBD_BUSY;
  }
  /* USER CODE END 15 */
  return result;
}

/**
  * @brief  CDC_ProcessTx
  *         Send buffered RX data as echo. Call this from CDC_Tx task.
  *         从环形缓冲区读取一包数据并发送。调用前 xSemaphoreTake 已确保有数据。
  * @retval None
  */
void CDC_ProcessTx(void)
{
  uint8_t buf[CDC_FS_MAX_PACKET_SIZE];
  uint16_t len = 0;

  /* 临界区内从环形缓冲区读取一包（不操作信号量，调用者已取走计数）*/
  taskENTER_CRITICAL();
  if (g_rx_ring_rd != g_rx_ring_wr)
  {
    memcpy(buf, g_rx_ring[g_rx_ring_rd].data, g_rx_ring[g_rx_ring_rd].len);
    len = g_rx_ring[g_rx_ring_rd].len;
    g_rx_ring_rd = (g_rx_ring_rd + 1) % CDC_RING_SIZE;
  }
  taskEXIT_CRITICAL();

  if (len > 0)
  {
    buf[len] = '\0';
    while (len > 0 && (buf[len-1] == '\r' || buf[len-1] == '\n'))
      buf[--len] = '\0';
    if(strcmp((const char *)buf,"ls")==0)
    {
      DIR file_dir;
      FILINFO file_info = {0};
      TCHAR lfname[256] = {0};
      file_info.lfname = lfname;
      file_info.lfsize = sizeof(lfname) / sizeof(TCHAR);
      FS_OpenDir("0:",&file_dir);
      while (1)
      {
        FS_ReadDir(&file_dir, &file_info);
        if (file_info.fname[0] == '\0')
          break;
        TCHAR *display_name = (file_info.lfname && file_info.lfname[0]) ? file_info.lfname : file_info.fname;
        while (CDC_Transmit_FS((uint8_t *)display_name, strlen(display_name)) != USBD_OK)
          taskYIELD();
        while (CDC_Transmit_FS((uint8_t *)"\r\n", 2) != USBD_OK)
          taskYIELD();
      }
    }
    else if(strcmp((const char *)buf,"format")==0)
    {
      extern FATFS fs;
      f_mount(NULL, "0:", 1);
      f_mkfs("0:", 0, 0);
      f_mount(&fs, "0:", 1);
      while (CDC_Transmit_FS((uint8_t *)"OK\r\n", 4) != USBD_OK)
        taskYIELD();
    }
    else
      CDC_Transmit_FS(buf, len);
  }
}

/**
  * @brief  CDC_Init
  *         创建 CDC 接收计数信号量。
  *         在 vTaskStartScheduler() 之前调用。
  * @retval None
  */
void CDC_Init(void)
{
  g_rx_sem = xSemaphoreCreateCounting(CDC_RING_SIZE - 1, 0);
}

/**
  * @brief  CDC_IsConnected
  *         检查 PC 端是否已打开 CDC 虚拟串口（DTR 置位）。
  *         DTR 状态通过 CDC_Control_FS 回调更新，同时直接从
  *         CDC 句柄的 CmdLength 读取（ST 库自动保存）。
  * @retval 1=已连接，0=未连接
  */
uint8_t CDC_IsConnected(void)
{
  if (g_cdc_connected) return 1;
  /* 后备：直接从 ST 库保存的寄存器值读取 */
  if (g_hcdc != NULL && (g_hcdc->CmdLength & 0x01)) {
    g_cdc_connected = 1;
    return 1;
  }
  return 0;
}

/**
  * @brief  CDC_IsReady
  *         检查 USB CDC 设备是否已枚举就绪（设备已配置，句柄有效）。
  *         不依赖 DTR 信号，适用于串口软件不置位 DTR 的情况。
  * @retval 1=就绪，0=未就绪
  */
uint8_t CDC_IsReady(void)
{
  return (g_hcdc != NULL &&
          hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED) ? 1 : 0;
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

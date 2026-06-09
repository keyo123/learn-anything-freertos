/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_cdc_if.h
  * @brief          : Header for usbd_cdc_if.c file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __USBD_CDC_IF__H__
#define __USBD_CDC_IF__H__

#ifdef __cplusplus
 extern "C" {
#endif

#include "usbd_cdc.h"

/* Exported functions -------------------------------------------------------*/
int8_t CDC_Control_FS(uint8_t cmd, uint8_t *pbuf, uint16_t length);
int8_t CDC_DataTx_FS(void);
int8_t CDC_DataRx_FS(uint8_t *pbuf, uint32_t *len);
int8_t CDC_Transmit_FS(uint8_t *Buf, uint16_t Len);
void CDC_CacheHandle(void);
void CDC_CacheHandleDirect(void *hcdc);
void CDC_Init(void);
void CDC_ProcessTx(void);
uint8_t CDC_IsConnected(void);
uint8_t CDC_IsReady(void);

/* CDC Interface callback structure */
extern USBD_CDC_ItfTypeDef USBD_CDC_fops_FS;

/* User can use this variable to control CDC operations */
extern uint8_t UserRxBufferFS[CDC_FS_MAX_PACKET_SIZE];
extern uint8_t UserTxBufferFS[CDC_FS_MAX_PACKET_SIZE];

/* FreeRTOS 计数信号量句柄 — CDC 接收数据计数 */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
extern SemaphoreHandle_t g_rx_sem;

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CDC_IF__H__ */

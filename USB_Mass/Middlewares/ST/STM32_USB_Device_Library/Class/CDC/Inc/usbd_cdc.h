/**
  ******************************************************************************
  * @file    usbd_cdc.h
  * @author  MCD Application Team
  * @brief   Header for usbd_cdc.c file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      www.st.com/SLA0044
  *
  ******************************************************************************
  */

#ifndef __USBD_CDC_H
#define __USBD_CDC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_def.h"

/** @addtogroup STM32_USBD_DEVICE_LIBRARY
  * @{
  */

/** @defgroup USBD_CDC
  * @brief This file is the Header file for usbd_cdc.c
  * @{
  */

/** @defgroup USBD_CDC_Exported_Defines
  * @{
  */

/* CDC Class Codes */
#define USB_CDC_CLASS                      0x02U
#define USB_CDC_COMM_SUBCLASS              0x02U
#define USB_CDC_COMM_PROTOCOL              0x01U
#define USB_CDC_DATA_CLASS                 0x0AU
#define USB_CDC_DATA_SUBCLASS              0x00U
#define USB_CDC_DATA_PROTOCOL              0x00U

/* CDC Communication Interface Class Requests */
#define CDC_SEND_ENCAPSULATED_COMMAND       0x00U
#define CDC_GET_ENCAPSULATED_RESPONSE       0x01U
#define CDC_SET_COMM_FEATURE                0x02U
#define CDC_GET_COMM_FEATURE                0x03U
#define CDC_CLEAR_COMM_FEATURE              0x04U
#define CDC_SET_AUX_LINE_STATE              0x10U
#define CDC_SET_HOOK_STATE                  0x11U
#define CDC_PULSE_SETUP                     0x12U
#define CDC_SEND_PULSE                      0x13U
#define CDC_SET_PULSE_TIME                  0x14U
#define CDC_RING_AUX_JACK                   0x15U
#define CDC_SET_LINE_CODING                 0x20U
#define CDC_GET_LINE_CODING                 0x21U
#define CDC_SET_CONTROL_LINE_STATE          0x22U
#define CDC_SEND_BREAK                      0x23U
#define CDC_SET_RINGER_PARMS                0x30U
#define CDC_GET_RINGER_PARMS                0x31U
#define CDC_SET_OPERATION_PARMS             0x32U
#define CDC_GET_OPERATION_PARMS             0x33U
#define CDC_SET_LINE_PARMS                  0x34U
#define CDC_GET_LINE_PARMS                  0x35U
#define CDC_DIAL_DIGITS                     0x36U
#define CDC_SET_UNIT_PARAMETER              0x37U
#define CDC_GET_UNIT_PARAMETER              0x38U
#define CDC_SET_FAX_MODE                    0x40U
#define CDC_GET_FAX_MODE                    0x41U
#define CDC_SET_FAX_PARMS                   0x42U
#define CDC_GET_FAX_PARMS                   0x43U
#define CDC_SET_NSF_FAX                     0x44U
#define CDC_GET_NSF_FAX                     0x45U
#define CDC_SET_T30_PARMS                   0x46U
#define CDC_GET_T30_PARMS                   0x47U
#define CDC_GET_FAX_XMIT_RATE               0x48U
#define CDC_SET_FAX_TIMEOUT_PARMS           0x49U
#define CDC_GET_FAX_TIMEOUT_PARMS           0x4AU
#define CDC_SET_SERIAL_OPERATION            0x50U
#define CDC_GET_SERIAL_OPERATION            0x51U
#define CDC_SET_TRANSMITTER_PARMS           0x52U
#define CDC_GET_TRANSMITTER_PARMS           0x53U
#define CDC_SET_RECEIVER_PARMS              0x54U
#define CDC_GET_RECEIVER_PARMS              0x55U

/* CDC Functional Descriptor Types */
#define CDC_CS_INTERFACE                    0x24U
#define CDC_HEADER_TYPE                     0x00U
#define CDC_CALL_MANAGEMENT_TYPE            0x01U
#define CDC_ACM_TYPE                        0x02U
#define CDC_UNION_TYPE                      0x06U

/* Line Coding Structure */
#define CDC_LINE_CODING_SIZE                0x07U

/* Control Line State bits */
#define CDC_CLS_DTR_PRESENT                 0x01U
#define CDC_CLS_RTS_CARRIER                 0x02U

/* Max Packet Sizes */
#define CDC_FS_MAX_PACKET_SIZE              0x40U
#define CDC_HS_MAX_PACKET_SIZE              0x200U

/* Endpoint addresses */
#define CDC_IN_EP                           0x83U
#define CDC_OUT_EP                          0x03U
#define CDC_CMD_EP                          0x82U

/* Config descriptor size for CDC interface */
#define USB_CDC_CONFIG_DESC_SIZ             67U

/**
  * @}
  */

/** @defgroup USBD_CDC_Exported_Types
  * @{
  */

typedef struct _USBD_CDC_Itf
{
  int8_t (*pIf_Control)(uint8_t cmd, uint8_t *pbuf, uint16_t length);
  int8_t (*pIf_DataTx)(void);
  int8_t (*pIf_DataRx)(uint8_t *pbuf, uint32_t *len);
} USBD_CDC_ItfTypeDef;

typedef struct _USBD_CDC_HandleTypeDef
{
  uint32_t data[CDC_FS_MAX_PACKET_SIZE / 4U];
  uint8_t  CmdOpCode;
  uint8_t  CmdLength;
  uint8_t  NotificationBuffer[8];
  uint8_t  LineCoding[CDC_LINE_CODING_SIZE];
  volatile uint8_t  TxState;
  volatile uint8_t  RxState;
  uint16_t RxLength;
} USBD_CDC_HandleTypeDef;

/**
  * @}
  */

/** @defgroup USBD_CDC_Exported_Variables
  * @{
  */
extern USBD_ClassTypeDef USBD_CDC;
#define USBD_CDC_CLASS    &USBD_CDC
/**
  * @}
  */

/** @defgroup USBD_CDC_Exported_Functions
  * @{
  */
uint8_t USBD_CDC_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_CDC_ItfTypeDef *fops);
uint8_t USBD_CDC_TransmitPacket(USBD_HandleTypeDef *pdev);
uint8_t USBD_CDC_ReceivePacket(USBD_HandleTypeDef *pdev);
/**
  * @}
  */

/** @defgroup USBD_CDC_Exported_FunctionsPrototype
  * @{
  */
uint8_t USBD_CDC_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
uint8_t USBD_CDC_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
uint8_t USBD_CDC_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
uint8_t USBD_CDC_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
uint8_t USBD_CDC_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
uint8_t *USBD_CDC_GetFSCfgDesc(uint16_t *length);
uint8_t *USBD_CDC_GetHSCfgDesc(uint16_t *length);
uint8_t *USBD_CDC_GetOtherSpeedCfgDesc(uint16_t *length);
uint8_t *USBD_CDC_GetDeviceQualifierDescriptor(uint16_t *length);
/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif

#endif /* __USBD_CDC_H */

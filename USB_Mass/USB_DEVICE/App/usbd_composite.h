/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_composite.h
  * @brief          : Header for usbd_composite.c file.
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef __USBD_COMPOSITE_H
#define __USBD_COMPOSITE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "usbd_core.h"
#include "usbd_msc.h"
#include "usbd_cdc.h"
#include "usbd_storage_if.h"

/* Composite configuration descriptor size:
   Config(9) + MSC_IF(9) + MSC_EP_OUT(7) + MSC_EP_IN(7) + IAD(8)
   + CDC_Comm_IF(9) + Header_FD(5) + CallMgmt_FD(5) + ACM_FD(4) + Union_FD(5)
   + CDC_Notif_EP(7) + CDC_Data_IF(9) + CDC_DataIn_EP(7) + CDC_DataOut_EP(7) = 98 */
#define USBD_COMPOSITE_CONFIG_DESC_SIZ      98

/* Interface numbers */
#define COMPOSITE_MSC_INTERFACE             0
#define COMPOSITE_CDC_COMM_INTERFACE        1
#define COMPOSITE_CDC_DATA_INTERFACE        2

/* Composite class handle */
typedef struct
{
  USBD_MSC_BOT_HandleTypeDef  msc;
  USBD_CDC_HandleTypeDef      cdc;
  USBD_StorageTypeDef         *storage_fops;
  USBD_CDC_ItfTypeDef         *cdc_fops;
} USBD_Composite_HandleTypeDef;

/* Exported class */
extern USBD_ClassTypeDef USBD_Composite;

uint8_t USBD_Composite_RegisterStorage(USBD_HandleTypeDef *pdev,
                                        USBD_StorageTypeDef *fops);
uint8_t USBD_Composite_RegisterCDC(USBD_HandleTypeDef *pdev,
                                    USBD_CDC_ItfTypeDef *fops);
USBD_CDC_HandleTypeDef *USBD_Composite_GetCDC(USBD_HandleTypeDef *pdev);

#ifdef __cplusplus
}
#endif

#endif /* __USBD_COMPOSITE_H */

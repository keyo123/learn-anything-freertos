/**
  ******************************************************************************
  * @file    usbd_cdc.c
  * @author  MCD Application Team
  * @brief   This file provides the CDC core functions.
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

#include "usbd_cdc.h"
#include "usbd_ctlreq.h"
#include "usbd_ioreq.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/

/* CDC class callbacks */
static uint8_t USBD_CDC_Init_Post(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t USBD_CDC_DeInit_Post(USBD_HandleTypeDef *pdev, uint8_t cfgidx);

USBD_ClassTypeDef USBD_CDC =
{
  USBD_CDC_Init,
  USBD_CDC_DeInit,
  USBD_CDC_Setup,
  NULL, /* EP0_TxSent */
  NULL, /* EP0_RxReady */
  USBD_CDC_DataIn,
  USBD_CDC_DataOut,
  NULL, /* SOF */
  NULL,
  NULL,
  USBD_CDC_GetHSCfgDesc,
  USBD_CDC_GetFSCfgDesc,
  USBD_CDC_GetOtherSpeedCfgDesc,
  USBD_CDC_GetDeviceQualifierDescriptor,
};

/* USB CDC device Configuration Descriptor */
__ALIGN_BEGIN uint8_t USBD_CDC_CfgHSDesc[USB_CDC_CONFIG_DESC_SIZ] __ALIGN_END =
{
  /* Configuration Descriptor */
  0x09,                                    /* bLength */
  USB_DESC_TYPE_CONFIGURATION,             /* bDescriptorType */
  USB_CDC_CONFIG_DESC_SIZ,                 /* wTotalLength */
  0x00,
  0x02,                                    /* bNumInterfaces */
  0x01,                                    /* bConfigurationValue */
  0x00,                                    /* iConfiguration */
  0xC0,                                    /* bmAttributes (Self Powered) */
  0x32,                                    /* bMaxPower (100mA) */

  /* Interface Descriptor - Communication Control */
  0x09,                                    /* bLength */
  USB_DESC_TYPE_INTERFACE,                 /* bDescriptorType */
  0x00,                                    /* bInterfaceNumber */
  0x00,                                    /* bAlternateSetting */
  0x01,                                    /* bNumEndpoints */
  USB_CDC_CLASS,                           /* bInterfaceClass (CDC) */
  USB_CDC_COMM_SUBCLASS,                   /* bInterfaceSubClass (ACM) */
  USB_CDC_COMM_PROTOCOL,                   /* bInterfaceProtocol (AT) */
  0x00,                                    /* iInterface */

  /* CDC Header Functional Descriptor */
  0x05,                                    /* bLength */
  CDC_CS_INTERFACE,                        /* bDescriptorType (CS_INTERFACE) */
  CDC_HEADER_TYPE,                         /* bDescriptorSubType */
  0x10,                                    /* bcdCDC (1.10) */
  0x01,

  /* CDC Call Management Functional Descriptor */
  0x05,                                    /* bLength */
  CDC_CS_INTERFACE,                        /* bDescriptorType */
  CDC_CALL_MANAGEMENT_TYPE,                /* bDescriptorSubType */
  0x00,                                    /* bmCapabilities */
  0x01,                                    /* bDataInterface */

  /* CDC ACM Functional Descriptor */
  0x04,                                    /* bLength */
  CDC_CS_INTERFACE,                        /* bDescriptorType */
  CDC_ACM_TYPE,                            /* bDescriptorSubType */
  0x02,                                    /* bmCapabilities (Line Coding + Serial State) */

  /* CDC Union Functional Descriptor */
  0x05,                                    /* bLength */
  CDC_CS_INTERFACE,                        /* bDescriptorType */
  CDC_UNION_TYPE,                          /* bDescriptorSubType */
  0x00,                                    /* bMasterInterface (CDC Control) */
  0x01,                                    /* bSlaveInterface (CDC Data) */

  /* Endpoint Descriptor - Notification */
  0x07,                                    /* bLength */
  USB_DESC_TYPE_ENDPOINT,                  /* bDescriptorType */
  CDC_CMD_EP,                              /* bEndpointAddress (IN) */
  USBD_EP_TYPE_INTR,                       /* bmAttributes (Interrupt) */
  LOBYTE(8),                               /* wMaxPacketSize */
  HIBYTE(8),
  0x10,                                    /* bInterval (16ms) */

  /* Interface Descriptor - Data Interface */
  0x09,                                    /* bLength */
  USB_DESC_TYPE_INTERFACE,                 /* bDescriptorType */
  0x01,                                    /* bInterfaceNumber */
  0x00,                                    /* bAlternateSetting */
  0x02,                                    /* bNumEndpoints */
  USB_CDC_DATA_CLASS,                      /* bInterfaceClass (CDC Data) */
  USB_CDC_DATA_SUBCLASS,                   /* bInterfaceSubClass */
  USB_CDC_DATA_PROTOCOL,                   /* bInterfaceProtocol */
  0x00,                                    /* iInterface */

  /* Endpoint Descriptor - Data OUT */
  0x07,                                    /* bLength */
  USB_DESC_TYPE_ENDPOINT,                  /* bDescriptorType */
  CDC_OUT_EP,                              /* bEndpointAddress (OUT) */
  USBD_EP_TYPE_BULK,                       /* bmAttributes (Bulk) */
  LOBYTE(CDC_FS_MAX_PACKET_SIZE),          /* wMaxPacketSize */
  HIBYTE(CDC_FS_MAX_PACKET_SIZE),
  0x00,                                    /* bInterval */

  /* Endpoint Descriptor - Data IN */
  0x07,                                    /* bLength */
  USB_DESC_TYPE_ENDPOINT,                  /* bDescriptorType */
  CDC_IN_EP,                               /* bEndpointAddress (IN) */
  USBD_EP_TYPE_BULK,                       /* bmAttributes (Bulk) */
  LOBYTE(CDC_FS_MAX_PACKET_SIZE),          /* wMaxPacketSize */
  HIBYTE(CDC_FS_MAX_PACKET_SIZE),
  0x00,                                    /* bInterval */
};

__ALIGN_BEGIN uint8_t USBD_CDC_CfgFSDesc[USB_CDC_CONFIG_DESC_SIZ] __ALIGN_END =
{
  /* Configuration Descriptor */
  0x09,                                    /* bLength */
  USB_DESC_TYPE_CONFIGURATION,             /* bDescriptorType */
  USB_CDC_CONFIG_DESC_SIZ,                 /* wTotalLength */
  0x00,
  0x02,                                    /* bNumInterfaces */
  0x01,                                    /* bConfigurationValue */
  0x00,                                    /* iConfiguration */
  0xC0,                                    /* bmAttributes (Self Powered) */
  0x32,                                    /* bMaxPower (100mA) */

  /* Interface Descriptor - Communication Control */
  0x09,                                    /* bLength */
  USB_DESC_TYPE_INTERFACE,                 /* bDescriptorType */
  0x00,                                    /* bInterfaceNumber */
  0x00,                                    /* bAlternateSetting */
  0x01,                                    /* bNumEndpoints */
  USB_CDC_CLASS,                           /* bInterfaceClass (CDC) */
  USB_CDC_COMM_SUBCLASS,                   /* bInterfaceSubClass (ACM) */
  USB_CDC_COMM_PROTOCOL,                   /* bInterfaceProtocol (AT) */
  0x00,                                    /* iInterface */

  /* CDC Header Functional Descriptor */
  0x05,                                    /* bLength */
  CDC_CS_INTERFACE,                        /* bDescriptorType (CS_INTERFACE) */
  CDC_HEADER_TYPE,                         /* bDescriptorSubType */
  0x10,                                    /* bcdCDC (1.10) */
  0x01,

  /* CDC Call Management Functional Descriptor */
  0x05,                                    /* bLength */
  CDC_CS_INTERFACE,                        /* bDescriptorType */
  CDC_CALL_MANAGEMENT_TYPE,                /* bDescriptorSubType */
  0x00,                                    /* bmCapabilities */
  0x01,                                    /* bDataInterface */

  /* CDC ACM Functional Descriptor */
  0x04,                                    /* bLength */
  CDC_CS_INTERFACE,                        /* bDescriptorType */
  CDC_ACM_TYPE,                            /* bDescriptorSubType */
  0x02,                                    /* bmCapabilities (Line Coding + Serial State) */

  /* CDC Union Functional Descriptor */
  0x05,                                    /* bLength */
  CDC_CS_INTERFACE,                        /* bDescriptorType */
  CDC_UNION_TYPE,                          /* bDescriptorSubType */
  0x00,                                    /* bMasterInterface (CDC Control) */
  0x01,                                    /* bSlaveInterface (CDC Data) */

  /* Endpoint Descriptor - Notification */
  0x07,                                    /* bLength */
  USB_DESC_TYPE_ENDPOINT,                  /* bDescriptorType */
  CDC_CMD_EP,                              /* bEndpointAddress (IN) */
  USBD_EP_TYPE_INTR,                       /* bmAttributes (Interrupt) */
  LOBYTE(8),                               /* wMaxPacketSize */
  HIBYTE(8),
  0x10,                                    /* bInterval (16ms) */

  /* Interface Descriptor - Data Interface */
  0x09,                                    /* bLength */
  USB_DESC_TYPE_INTERFACE,                 /* bDescriptorType */
  0x01,                                    /* bInterfaceNumber */
  0x00,                                    /* bAlternateSetting */
  0x02,                                    /* bNumEndpoints */
  USB_CDC_DATA_CLASS,                      /* bInterfaceClass (CDC Data) */
  USB_CDC_DATA_SUBCLASS,                   /* bInterfaceSubClass */
  USB_CDC_DATA_PROTOCOL,                   /* bInterfaceProtocol */
  0x00,                                    /* iInterface */

  /* Endpoint Descriptor - Data OUT */
  0x07,                                    /* bLength */
  USB_DESC_TYPE_ENDPOINT,                  /* bDescriptorType */
  CDC_OUT_EP,                              /* bEndpointAddress (OUT) */
  USBD_EP_TYPE_BULK,                       /* bmAttributes (Bulk) */
  LOBYTE(CDC_FS_MAX_PACKET_SIZE),          /* wMaxPacketSize */
  HIBYTE(CDC_FS_MAX_PACKET_SIZE),
  0x00,                                    /* bInterval */

  /* Endpoint Descriptor - Data IN */
  0x07,                                    /* bLength */
  USB_DESC_TYPE_ENDPOINT,                  /* bDescriptorType */
  CDC_IN_EP,                               /* bEndpointAddress (IN) */
  USBD_EP_TYPE_BULK,                       /* bmAttributes (Bulk) */
  LOBYTE(CDC_FS_MAX_PACKET_SIZE),          /* wMaxPacketSize */
  HIBYTE(CDC_FS_MAX_PACKET_SIZE),
  0x00,                                    /* bInterval */
};

__ALIGN_BEGIN uint8_t USBD_CDC_OtherSpeedCfgDesc[USB_CDC_CONFIG_DESC_SIZ] __ALIGN_END =
{
  0x09,                                    /* bLength */
  USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION,
  USB_CDC_CONFIG_DESC_SIZ,
  0x00,
  0x02,
  0x01,
  0x00,
  0xC0,
  0x32,

  /* Interface Descriptor */
  0x09,
  USB_DESC_TYPE_INTERFACE,
  0x00,
  0x00,
  0x01,
  USB_CDC_CLASS,
  USB_CDC_COMM_SUBCLASS,
  USB_CDC_COMM_PROTOCOL,
  0x00,

  0x05,
  CDC_CS_INTERFACE,
  CDC_HEADER_TYPE,
  0x10,
  0x01,

  0x05,
  CDC_CS_INTERFACE,
  CDC_CALL_MANAGEMENT_TYPE,
  0x00,
  0x01,

  0x04,
  CDC_CS_INTERFACE,
  CDC_ACM_TYPE,
  0x02,

  0x05,
  CDC_CS_INTERFACE,
  CDC_UNION_TYPE,
  0x00,
  0x01,

  0x07,
  USB_DESC_TYPE_ENDPOINT,
  CDC_CMD_EP,
  USBD_EP_TYPE_INTR,
  LOBYTE(8),
  HIBYTE(8),
  0x10,

  0x09,
  USB_DESC_TYPE_INTERFACE,
  0x01,
  0x00,
  0x02,
  USB_CDC_DATA_CLASS,
  USB_CDC_DATA_SUBCLASS,
  USB_CDC_DATA_PROTOCOL,
  0x00,

  0x07,
  USB_DESC_TYPE_ENDPOINT,
  CDC_OUT_EP,
  USBD_EP_TYPE_BULK,
  LOBYTE(CDC_FS_MAX_PACKET_SIZE),
  HIBYTE(CDC_FS_MAX_PACKET_SIZE),
  0x00,

  0x07,
  USB_DESC_TYPE_ENDPOINT,
  CDC_IN_EP,
  USBD_EP_TYPE_BULK,
  LOBYTE(CDC_FS_MAX_PACKET_SIZE),
  HIBYTE(CDC_FS_MAX_PACKET_SIZE),
  0x00,
};

/* USB Standard Device Descriptor */
__ALIGN_BEGIN uint8_t USBD_CDC_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
{
  USB_LEN_DEV_QUALIFIER_DESC,
  USB_DESC_TYPE_DEVICE_QUALIFIER,
  0x00,
  0x02,
  0x00,
  0x00,
  0x00,
  0x40,
  0x01,
  0x00,
};

/* Private functions ---------------------------------------------------------*/

/**
  * @brief  USBD_CDC_Init
  *         Initialize the CDC interface
  * @param  pdev: device instance
  * @param  cfgidx: configuration index
  * @retval status
  */
uint8_t USBD_CDC_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  uint8_t ret = 0U;
  USBD_CDC_HandleTypeDef *hcdc;

  if (pdev->dev_speed == USBD_SPEED_HIGH)
  {
    /* Allocate high-speed buffer */
    pdev->pClassData = USBD_malloc(sizeof(USBD_CDC_HandleTypeDef) + 2U * sizeof(uint32_t));
  }
  else
  {
    /* Allocate full-speed buffer */
    pdev->pClassData = USBD_malloc(sizeof(USBD_CDC_HandleTypeDef));
  }

  if (pdev->pClassData == NULL)
  {
    ret = 1U;
  }
  else
  {
    hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassData;

    /* Init physical interface */
    if (USBD_CDC_RegisterInterface(pdev, NULL) != USBD_OK)
    {
      ret = 1U;
    }
    else
    {
      /* Open EP IN */
      USBD_LL_OpenEP(pdev, CDC_IN_EP, USBD_EP_TYPE_BULK, CDC_FS_MAX_PACKET_SIZE);
      pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 1U;

      /* Open EP OUT */
      USBD_LL_OpenEP(pdev, CDC_OUT_EP, USBD_EP_TYPE_BULK, CDC_FS_MAX_PACKET_SIZE);
      pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 1U;

      /* Open CMD EP */
      USBD_LL_OpenEP(pdev, CDC_CMD_EP, USBD_EP_TYPE_INTR, 8U);
      pdev->ep_in[CDC_CMD_EP & 0xFU].is_used = 1U;

      /* Initialize Line Coding parameters */
      hcdc->LineCoding[0] = 0x00; /* baud rate: 115200 */
      hcdc->LineCoding[1] = 0xC2;
      hcdc->LineCoding[2] = 0x01;
      hcdc->LineCoding[3] = 0x00;
      hcdc->LineCoding[4] = 0x00; /* stop bits: 1 */
      hcdc->LineCoding[5] = 0x00; /* parity: none */
      hcdc->LineCoding[6] = 0x08; /* data bits: 8 */

      hcdc->TxState = 0U;
      hcdc->RxState = 0U;

      /* Prepare Out endpoint to receive next packet */
      USBD_LL_PrepareReceive(pdev, CDC_OUT_EP, (uint8_t *)hcdc->data, CDC_FS_MAX_PACKET_SIZE);
    }
  }

  return ret;
}

/**
  * @brief  USBD_CDC_DeInit
  *         DeInitialize the CDC layer
  * @param  pdev: device instance
  * @param  cfgidx: configuration index
  * @retval status
  */
uint8_t USBD_CDC_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  /* Close EP IN */
  USBD_LL_CloseEP(pdev, CDC_IN_EP);
  pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 0U;

  /* Close EP OUT */
  USBD_LL_CloseEP(pdev, CDC_OUT_EP);
  pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 0U;

  /* Close CMD EP */
  USBD_LL_CloseEP(pdev, CDC_CMD_EP);
  pdev->ep_in[CDC_CMD_EP & 0xFU].is_used = 0U;

  /* Free class resources */
  if (pdev->pClassData != NULL)
  {
    USBD_free(pdev->pClassData);
    pdev->pClassData = NULL;
  }

  return USBD_OK;
}

/**
  * @brief  USBD_CDC_Setup
  *         Handles CDC-specific requests
  * @param  pdev: device instance
  * @param  req: USB setup request
  * @retval status
  */
uint8_t USBD_CDC_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassData;
  uint8_t ifalt = 0U;
  uint16_t status_info = 0U;
  USBD_StatusTypeDef ret = USBD_OK;

  switch (req->bmRequest & USB_REQ_TYPE_MASK)
  {
    case USB_REQ_TYPE_CLASS:
      /* Class-specific requests */
      switch (req->bRequest)
      {
        case CDC_SET_LINE_CODING:
          if (req->wLength == CDC_LINE_CODING_SIZE)
          {
            /* Receive line coding data on EP0 (control transfer data stage) */
            USBD_CtlPrepareRx(pdev, (uint8_t *)hcdc->LineCoding, CDC_LINE_CODING_SIZE);
            hcdc->CmdOpCode = 0xFFU;
          }
          else
          {
            ret = USBD_FAIL;
          }
          break;

        case CDC_GET_LINE_CODING:
          /* Send line coding */
          USBD_CtlSendData(pdev, (uint8_t *)hcdc->LineCoding, CDC_LINE_CODING_SIZE);
          break;

        case CDC_SET_CONTROL_LINE_STATE:
          /* Store control line state */
          hcdc->CmdOpCode = CDC_SET_CONTROL_LINE_STATE;
          hcdc->CmdLength = (uint8_t)(req->wValue);
          /* Notify application layer via registered callback */
          {
            USBD_CDC_ItfTypeDef *itf = (USBD_CDC_ItfTypeDef *)pdev->pUserData;
            if ((itf != NULL) && (itf->pIf_Control != NULL))
            {
              itf->pIf_Control(CDC_SET_CONTROL_LINE_STATE, (uint8_t *)&req->wValue, 2U);
            }
          }
          break;

        case CDC_SEND_BREAK:
          /* Not implemented */
          break;

        default:
          ret = USBD_FAIL;
          break;
      }
      break;

    case USB_REQ_TYPE_STANDARD:
      /* Standard requests */
      switch (req->bRequest)
      {
        case USB_REQ_GET_INTERFACE:
          USBD_CtlSendData(pdev, (uint8_t *)&ifalt, 1U);
          break;

        case USB_REQ_SET_INTERFACE:
          break;

        case USB_REQ_GET_STATUS:
          /* Interface status */
          if (pdev->dev_state == USBD_STATE_CONFIGURED)
          {
            USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
          }
          else
          {
            USBD_CtlError(pdev, req);
            ret = USBD_FAIL;
          }
          break;

        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
          break;
      }
      break;

    default:
      ret = USBD_FAIL;
      break;
  }

  return ret;
}

/**
  * @brief  USBD_CDC_DataIn
  *         Handle data IN transmission complete
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
uint8_t USBD_CDC_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassData;

  if (epnum == (CDC_IN_EP & 0x7FU))
  {
    hcdc->TxState = 0U;

    if (USBD_CDC_RegisterInterface(pdev, NULL) == USBD_OK)
    {
      /* Notify application that data has been sent */
      USBD_CDC_ItfTypeDef *itf = (USBD_CDC_ItfTypeDef *)pdev->pUserData;
      if ((itf != NULL) && (itf->pIf_DataTx != NULL))
      {
        itf->pIf_DataTx();
      }
    }

    return USBD_OK;
  }

  return USBD_FAIL;
}

/**
  * @brief  USBD_CDC_DataOut
  *         Handle data OUT reception complete
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
uint8_t USBD_CDC_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassData;

  if (epnum == (CDC_OUT_EP & 0x7FU))
  {
    /* Store received data length */
    hcdc->RxLength = USBD_LL_GetRxDataSize(pdev, epnum);

    /* Prepare for next reception BEFORE callback (arm OUT endpoint first) */
    USBD_LL_PrepareReceive(pdev, CDC_OUT_EP, (uint8_t *)hcdc->data, CDC_FS_MAX_PACKET_SIZE);

    if (USBD_CDC_RegisterInterface(pdev, NULL) == USBD_OK)
    {
      /* Notify application */
      USBD_CDC_ItfTypeDef *itf = (USBD_CDC_ItfTypeDef *)pdev->pUserData;
      if ((itf != NULL) && (itf->pIf_DataRx != NULL))
      {
        itf->pIf_DataRx((uint8_t *)hcdc->data, &hcdc->RxLength);
      }
    }

    return USBD_OK;
  }

  return USBD_FAIL;
}

/**
  * @brief  USBD_CDC_RegisterInterface
  * @param  pdev: device instance
  * @param  fops: CDC interface callbacks
  * @retval status
  */
uint8_t USBD_CDC_RegisterInterface(USBD_HandleTypeDef *pdev, USBD_CDC_ItfTypeDef *fops)
{
  if (fops != NULL)
  {
    pdev->pUserData = fops;
  }

  return USBD_OK;
}

/**
  * @brief  USBD_CDC_TransmitPacket
  *         Send data over USB CDC IN endpoint
  * @param  pdev: device instance
  * @retval status
  */
uint8_t USBD_CDC_TransmitPacket(USBD_HandleTypeDef *pdev)
{
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassData;

  if (hcdc->TxState != 0U)
  {
    return USBD_BUSY;
  }

  hcdc->TxState = 1U;
  USBD_LL_Transmit(pdev, CDC_IN_EP, (uint8_t *)hcdc->data, hcdc->RxLength);

  return USBD_OK;
}

/**
  * @brief  USBD_CDC_ReceivePacket
  *         Prepare reception of next CDC data packet
  * @param  pdev: device instance
  * @retval status
  */
uint8_t USBD_CDC_ReceivePacket(USBD_HandleTypeDef *pdev)
{
  USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef *)pdev->pClassData;

  /* Suspend or Resume USB OUT process */
  if (hcdc->RxState == 0U)
  {
    USBD_LL_PrepareReceive(pdev, CDC_OUT_EP, (uint8_t *)hcdc->data, CDC_FS_MAX_PACKET_SIZE);
  }

  return USBD_OK;
}

/**
  * @brief  USBD_CDC_GetFSCfgDesc
  *         Return FS configuration descriptor
  * @param  length: pointer to data length variable
  * @retval pointer to descriptor buffer
  */
uint8_t *USBD_CDC_GetFSCfgDesc(uint16_t *length)
{
  *length = sizeof(USBD_CDC_CfgFSDesc);
  return USBD_CDC_CfgFSDesc;
}

/**
  * @brief  USBD_CDC_GetHSCfgDesc
  *         Return HS configuration descriptor
  * @param  length: pointer to data length variable
  * @retval pointer to descriptor buffer
  */
uint8_t *USBD_CDC_GetHSCfgDesc(uint16_t *length)
{
  *length = sizeof(USBD_CDC_CfgHSDesc);
  return USBD_CDC_CfgHSDesc;
}

/**
  * @brief  USBD_CDC_GetOtherSpeedCfgDesc
  *         Return other speed configuration descriptor
  * @param  length: pointer to data length variable
  * @retval pointer to descriptor buffer
  */
uint8_t *USBD_CDC_GetOtherSpeedCfgDesc(uint16_t *length)
{
  *length = sizeof(USBD_CDC_OtherSpeedCfgDesc);
  return USBD_CDC_OtherSpeedCfgDesc;
}

/**
  * @brief  USBD_CDC_GetDeviceQualifierDescriptor
  *         Return device qualifier descriptor
  * @param  length: pointer to data length variable
  * @retval pointer to descriptor buffer
  */
uint8_t *USBD_CDC_GetDeviceQualifierDescriptor(uint16_t *length)
{
  *length = sizeof(USBD_CDC_DeviceQualifierDesc);
  return USBD_CDC_DeviceQualifierDesc;
}

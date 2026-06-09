/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : usbd_composite.c
  * @brief          : Composite USB device (MSC + CDC) implementation
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "usbd_composite.h"
#include "usbd_ctlreq.h"
#include "usbd_ioreq.h"

/* USER CODE BEGIN INCLUDE */
#include "usbd_cdc_if.h"
#include "usbd_msc.h"
/* MSC 类回调函数 — 在 usbd_msc.c 中定义但未在头文件中声明 */
extern uint8_t USBD_MSC_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
extern uint8_t USBD_MSC_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
extern uint8_t USBD_MSC_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
/* USER CODE END INCLUDE */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
static USBD_StorageTypeDef *g_comp_storage_fops = NULL;
static USBD_CDC_ItfTypeDef *g_comp_cdc_fops = NULL;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
static uint8_t  Composite_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  Composite_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
static uint8_t  Composite_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
static uint8_t  Composite_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  Composite_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
static uint8_t  *Composite_GetHSCfgDesc(uint16_t *length);
static uint8_t  *Composite_GetFSCfgDesc(uint16_t *length);
static uint8_t  *Composite_GetOtherSpeedCfgDesc(uint16_t *length);
static uint8_t  *Composite_GetDeviceQualifierDescriptor(uint16_t *length);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/**
  * @brief  USBD_Composite_GetCDC
  *         Get CDC handle from composite handle
  * @param  pdev: device instance
  * @retval pointer to CDC handle, or NULL on failure
  */
USBD_CDC_HandleTypeDef *USBD_Composite_GetCDC(USBD_HandleTypeDef *pdev)
{
  USBD_Composite_HandleTypeDef *hcomp;

  if (pdev->pClassData == NULL)
  {
    return NULL;
  }

  hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;
  return &hcomp->cdc;
}

/* Composite class object */
USBD_ClassTypeDef USBD_Composite =
{
  Composite_Init,
  Composite_DeInit,
  Composite_Setup,
  NULL,  /* EP0_TxSent */
  NULL,  /* EP0_RxReady */
  Composite_DataIn,
  Composite_DataOut,
  NULL,  /* SOF */
  NULL,  /* IsoINIncomplete */
  NULL,  /* IsoOUTIncomplete */
  Composite_GetHSCfgDesc,
  Composite_GetFSCfgDesc,
  Composite_GetOtherSpeedCfgDesc,
  Composite_GetDeviceQualifierDescriptor,
};

/* USB Composite Device Configuration Descriptor (FS) */
__ALIGN_BEGIN uint8_t USBD_Composite_CfgFSDesc[USBD_COMPOSITE_CONFIG_DESC_SIZ] __ALIGN_END =
{
  /*----------------- Configuration Descriptor -----------------*/
  0x09,                                      /* bLength */
  USB_DESC_TYPE_CONFIGURATION,               /* bDescriptorType */
  USBD_COMPOSITE_CONFIG_DESC_SIZ,            /* wTotalLength (low) */
  0x00,                                      /* wTotalLength (high) */
  0x03,                                      /* bNumInterfaces: 3 */
  0x01,                                      /* bConfigurationValue */
  0x00,                                      /* iConfiguration */
  0xC0,                                      /* bmAttributes: Self Powered */
  0x32,                                      /* bMaxPower: 100mA */

  /******************** Interface 0: MSC BOT ********************/
  0x09,                                      /* bLength */
  USB_DESC_TYPE_INTERFACE,                   /* bDescriptorType */
  0x00,                                      /* bInterfaceNumber: 0 */
  0x00,                                      /* bAlternateSetting */
  0x02,                                      /* bNumEndpoints */
  0x08,                                      /* bInterfaceClass: MSC */
  0x06,                                      /* bInterfaceSubClass: SCSI */
  0x50,                                      /* bInterfaceProtocol: BOT */
  0x00,                                      /* iInterface */

  /* MSC EP OUT */
  0x07,                                      /* bLength */
  USB_DESC_TYPE_ENDPOINT,                    /* bDescriptorType */
  MSC_EPOUT_ADDR,                            /* bEndpointAddress: OUT 0x01 */
  USBD_EP_TYPE_BULK,                         /* bmAttributes: Bulk */
  LOBYTE(MSC_MAX_FS_PACKET),                 /* wMaxPacketSize */
  HIBYTE(MSC_MAX_FS_PACKET),
  0x00,                                      /* bInterval */

  /* MSC EP IN */
  0x07,                                      /* bLength */
  USB_DESC_TYPE_ENDPOINT,                    /* bDescriptorType */
  MSC_EPIN_ADDR,                             /* bEndpointAddress: IN 0x81 */
  USBD_EP_TYPE_BULK,                         /* bmAttributes: Bulk */
  LOBYTE(MSC_MAX_FS_PACKET),                 /* wMaxPacketSize */
  HIBYTE(MSC_MAX_FS_PACKET),
  0x00,                                      /* bInterval */

  /******************** IAD for CDC ********************/
  0x08,                                      /* bLength */
  0x0B,                                      /* bDescriptorType: IAD */
  COMPOSITE_CDC_COMM_INTERFACE,              /* bFirstInterface: 1 */
  0x02,                                      /* bInterfaceCount: 2 */
  USB_CDC_CLASS,                             /* bFunctionClass: CDC */
  USB_CDC_COMM_SUBCLASS,                     /* bFunctionSubClass: ACM */
  USB_CDC_COMM_PROTOCOL,                     /* bFunctionProtocol: AT */
  0x00,                                      /* iFunction */

  /******************** Interface 1: CDC Communication ********************/
  0x09,                                      /* bLength */
  USB_DESC_TYPE_INTERFACE,                   /* bDescriptorType */
  COMPOSITE_CDC_COMM_INTERFACE,              /* bInterfaceNumber: 1 */
  0x00,                                      /* bAlternateSetting */
  0x01,                                      /* bNumEndpoints */
  USB_CDC_CLASS,                             /* bInterfaceClass: CDC */
  USB_CDC_COMM_SUBCLASS,                     /* bInterfaceSubClass: ACM */
  USB_CDC_COMM_PROTOCOL,                     /* bInterfaceProtocol: AT */
  0x00,                                      /* iInterface */

  /* CDC Header Functional Descriptor */
  0x05,                                      /* bLength */
  CDC_CS_INTERFACE,                          /* bDescriptorType */
  CDC_HEADER_TYPE,                           /* bDescriptorSubType */
  0x10,                                      /* bcdCDC (1.10) low */
  0x01,                                      /* bcdCDC (1.10) high */

  /* CDC Call Management Functional Descriptor */
  0x05,                                      /* bLength */
  CDC_CS_INTERFACE,                          /* bDescriptorType */
  CDC_CALL_MANAGEMENT_TYPE,                  /* bDescriptorSubType */
  0x00,                                      /* bmCapabilities */
  COMPOSITE_CDC_DATA_INTERFACE,              /* bDataInterface: 2 */

  /* CDC ACM Functional Descriptor */
  0x04,                                      /* bLength */
  CDC_CS_INTERFACE,                          /* bDescriptorType */
  CDC_ACM_TYPE,                              /* bDescriptorSubType */
  0x02,                                      /* bmCapabilities: Line Coding + Serial State */

  /* CDC Union Functional Descriptor */
  0x05,                                      /* bLength */
  CDC_CS_INTERFACE,                          /* bDescriptorType */
  CDC_UNION_TYPE,                            /* bDescriptorSubType */
  COMPOSITE_CDC_COMM_INTERFACE,              /* bMasterInterface: 1 */
  COMPOSITE_CDC_DATA_INTERFACE,              /* bSlaveInterface: 2 */

  /* CDC Notification Endpoint (Interrupt IN) */
  0x07,                                      /* bLength */
  USB_DESC_TYPE_ENDPOINT,                    /* bDescriptorType */
  CDC_CMD_EP,                                /* bEndpointAddress: IN 0x82 */
  USBD_EP_TYPE_INTR,                         /* bmAttributes: Interrupt */
  LOBYTE(8),                                 /* wMaxPacketSize: 8 */
  HIBYTE(8),
  0x10,                                      /* bInterval: 16ms */

  /******************** Interface 2: CDC Data ********************/
  0x09,                                      /* bLength */
  USB_DESC_TYPE_INTERFACE,                   /* bDescriptorType */
  COMPOSITE_CDC_DATA_INTERFACE,              /* bInterfaceNumber: 2 */
  0x00,                                      /* bAlternateSetting */
  0x02,                                      /* bNumEndpoints */
  USB_CDC_DATA_CLASS,                        /* bInterfaceClass: CDC Data */
  USB_CDC_DATA_SUBCLASS,                     /* bInterfaceSubClass */
  USB_CDC_DATA_PROTOCOL,                     /* bInterfaceProtocol */
  0x00,                                      /* iInterface */

  /* CDC Data IN Endpoint (Bulk IN) */
  0x07,                                      /* bLength */
  USB_DESC_TYPE_ENDPOINT,                    /* bDescriptorType */
  CDC_IN_EP,                                 /* bEndpointAddress: IN 0x83 */
  USBD_EP_TYPE_BULK,                         /* bmAttributes: Bulk */
  LOBYTE(CDC_FS_MAX_PACKET_SIZE),            /* wMaxPacketSize: 64 */
  HIBYTE(CDC_FS_MAX_PACKET_SIZE),
  0x00,                                      /* bInterval */

  /* CDC Data OUT Endpoint (Bulk OUT) */
  0x07,                                      /* bLength */
  USB_DESC_TYPE_ENDPOINT,                    /* bDescriptorType */
  CDC_OUT_EP,                                /* bEndpointAddress: OUT 0x03 */
  USBD_EP_TYPE_BULK,                         /* bmAttributes: Bulk */
  LOBYTE(CDC_FS_MAX_PACKET_SIZE),            /* wMaxPacketSize: 64 */
  HIBYTE(CDC_FS_MAX_PACKET_SIZE),
  0x00,                                      /* bInterval */
};

__ALIGN_BEGIN uint8_t USBD_Composite_CfgHSDesc[USBD_COMPOSITE_CONFIG_DESC_SIZ] __ALIGN_END =
{
  0x09, USB_DESC_TYPE_CONFIGURATION,
  USBD_COMPOSITE_CONFIG_DESC_SIZ, 0x00,
  0x03, 0x01, 0x00, 0xC0, 0x32,

  /* MSC IF */
  0x09, USB_DESC_TYPE_INTERFACE, 0x00, 0x00, 0x02,
  0x08, 0x06, 0x50, 0x00,
  /* MSC EP OUT */
  0x07, USB_DESC_TYPE_ENDPOINT, MSC_EPOUT_ADDR, USBD_EP_TYPE_BULK,
  LOBYTE(MSC_MAX_HS_PACKET), HIBYTE(MSC_MAX_HS_PACKET), 0x00,
  /* MSC EP IN */
  0x07, USB_DESC_TYPE_ENDPOINT, MSC_EPIN_ADDR, USBD_EP_TYPE_BULK,
  LOBYTE(MSC_MAX_HS_PACKET), HIBYTE(MSC_MAX_HS_PACKET), 0x00,

  /* IAD */
  0x08, 0x0B, COMPOSITE_CDC_COMM_INTERFACE, 0x02,
  USB_CDC_CLASS, USB_CDC_COMM_SUBCLASS, USB_CDC_COMM_PROTOCOL, 0x00,

  /* CDC Comm IF */
  0x09, USB_DESC_TYPE_INTERFACE, COMPOSITE_CDC_COMM_INTERFACE, 0x00, 0x01,
  USB_CDC_CLASS, USB_CDC_COMM_SUBCLASS, USB_CDC_COMM_PROTOCOL, 0x00,
  /* Header FD */
  0x05, CDC_CS_INTERFACE, CDC_HEADER_TYPE, 0x10, 0x01,
  /* Call Mgmt FD */
  0x05, CDC_CS_INTERFACE, CDC_CALL_MANAGEMENT_TYPE, 0x00, COMPOSITE_CDC_DATA_INTERFACE,
  /* ACM FD */
  0x04, CDC_CS_INTERFACE, CDC_ACM_TYPE, 0x02,
  /* Union FD */
  0x05, CDC_CS_INTERFACE, CDC_UNION_TYPE, COMPOSITE_CDC_COMM_INTERFACE, COMPOSITE_CDC_DATA_INTERFACE,
  /* Notification EP */
  0x07, USB_DESC_TYPE_ENDPOINT, CDC_CMD_EP, USBD_EP_TYPE_INTR,
  LOBYTE(8), HIBYTE(8), 0x10,

  /* CDC Data IF */
  0x09, USB_DESC_TYPE_INTERFACE, COMPOSITE_CDC_DATA_INTERFACE, 0x00, 0x02,
  USB_CDC_DATA_CLASS, USB_CDC_DATA_SUBCLASS, USB_CDC_DATA_PROTOCOL, 0x00,
  /* CDC Data IN */
  0x07, USB_DESC_TYPE_ENDPOINT, CDC_IN_EP, USBD_EP_TYPE_BULK,
  LOBYTE(CDC_HS_MAX_PACKET_SIZE), HIBYTE(CDC_HS_MAX_PACKET_SIZE), 0x00,
  /* CDC Data OUT */
  0x07, USB_DESC_TYPE_ENDPOINT, CDC_OUT_EP, USBD_EP_TYPE_BULK,
  LOBYTE(CDC_HS_MAX_PACKET_SIZE), HIBYTE(CDC_HS_MAX_PACKET_SIZE), 0x00,
};

__ALIGN_BEGIN uint8_t USBD_Composite_OtherSpeedCfgDesc[USBD_COMPOSITE_CONFIG_DESC_SIZ] __ALIGN_END =
{
  0x09, USB_DESC_TYPE_OTHER_SPEED_CONFIGURATION,
  USBD_COMPOSITE_CONFIG_DESC_SIZ, 0x00,
  0x03, 0x01, 0x00, 0xC0, 0x32,

  /* MSC IF */
  0x09, USB_DESC_TYPE_INTERFACE, 0x00, 0x00, 0x02,
  0x08, 0x06, 0x50, 0x00,
  0x07, USB_DESC_TYPE_ENDPOINT, MSC_EPOUT_ADDR, USBD_EP_TYPE_BULK,
  LOBYTE(MSC_MAX_FS_PACKET), HIBYTE(MSC_MAX_FS_PACKET), 0x00,
  0x07, USB_DESC_TYPE_ENDPOINT, MSC_EPIN_ADDR, USBD_EP_TYPE_BULK,
  LOBYTE(MSC_MAX_FS_PACKET), HIBYTE(MSC_MAX_FS_PACKET), 0x00,

  /* IAD */
  0x08, 0x0B, COMPOSITE_CDC_COMM_INTERFACE, 0x02,
  USB_CDC_CLASS, USB_CDC_COMM_SUBCLASS, USB_CDC_COMM_PROTOCOL, 0x00,

  /* CDC Comm IF */
  0x09, USB_DESC_TYPE_INTERFACE, COMPOSITE_CDC_COMM_INTERFACE, 0x00, 0x01,
  USB_CDC_CLASS, USB_CDC_COMM_SUBCLASS, USB_CDC_COMM_PROTOCOL, 0x00,
  0x05, CDC_CS_INTERFACE, CDC_HEADER_TYPE, 0x10, 0x01,
  0x05, CDC_CS_INTERFACE, CDC_CALL_MANAGEMENT_TYPE, 0x00, COMPOSITE_CDC_DATA_INTERFACE,
  0x04, CDC_CS_INTERFACE, CDC_ACM_TYPE, 0x02,
  0x05, CDC_CS_INTERFACE, CDC_UNION_TYPE, COMPOSITE_CDC_COMM_INTERFACE, COMPOSITE_CDC_DATA_INTERFACE,
  0x07, USB_DESC_TYPE_ENDPOINT, CDC_CMD_EP, USBD_EP_TYPE_INTR,
  LOBYTE(8), HIBYTE(8), 0x10,

  0x09, USB_DESC_TYPE_INTERFACE, COMPOSITE_CDC_DATA_INTERFACE, 0x00, 0x02,
  USB_CDC_DATA_CLASS, USB_CDC_DATA_SUBCLASS, USB_CDC_DATA_PROTOCOL, 0x00,
  0x07, USB_DESC_TYPE_ENDPOINT, CDC_IN_EP, USBD_EP_TYPE_BULK,
  LOBYTE(CDC_FS_MAX_PACKET_SIZE), HIBYTE(CDC_FS_MAX_PACKET_SIZE), 0x00,
  0x07, USB_DESC_TYPE_ENDPOINT, CDC_OUT_EP, USBD_EP_TYPE_BULK,
  LOBYTE(CDC_FS_MAX_PACKET_SIZE), HIBYTE(CDC_FS_MAX_PACKET_SIZE), 0x00,
};

__ALIGN_BEGIN uint8_t USBD_Composite_DeviceQualifierDesc[USB_LEN_DEV_QUALIFIER_DESC] __ALIGN_END =
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
  * @brief  Composite_Init
  *         Init both MSC and CDC sub-classes
  * @param  pdev: device instance
  * @param  cfgidx: configuration index
  * @retval status
  */
static uint8_t Composite_Init(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_Composite_HandleTypeDef *hcomp;
  void *saved_data;
  void *saved_user;

  /* Allocate composite handle */
  pdev->pClassData = USBD_malloc(sizeof(USBD_Composite_HandleTypeDef));
  if (pdev->pClassData == NULL)
  {
    return USBD_FAIL;
  }
  hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;
  memset(hcomp, 0, sizeof(USBD_Composite_HandleTypeDef));

  /* Retrieve pre-registered callbacks */
  hcomp->storage_fops = g_comp_storage_fops;
  hcomp->cdc_fops = g_comp_cdc_fops;

  /* Save current pClassData/pUserData (pointing to composite handle) */
  saved_data = pdev->pClassData;
  saved_user = pdev->pUserData;

  /* ==================== MSC Init ==================== */
  pdev->pClassData = (void *)&hcomp->msc;
  pdev->pUserData = (void *)hcomp->storage_fops;

  /* Open MSC endpoints */
  USBD_LL_OpenEP(pdev, MSC_EPOUT_ADDR, USBD_EP_TYPE_BULK, MSC_MAX_FS_PACKET);
  pdev->ep_out[MSC_EPOUT_ADDR & 0xFU].is_used = 1U;

  USBD_LL_OpenEP(pdev, MSC_EPIN_ADDR, USBD_EP_TYPE_BULK, MSC_MAX_FS_PACKET);
  pdev->ep_in[MSC_EPIN_ADDR & 0xFU].is_used = 1U;

  /* Init MSC BOT layer (uses pClassData as MSC handle) */
  MSC_BOT_Init(pdev);

  /* ==================== CDC Init ==================== */
  pdev->pClassData = (void *)&hcomp->cdc;

  /* Open CDC endpoints */
  USBD_LL_OpenEP(pdev, CDC_IN_EP, USBD_EP_TYPE_BULK, CDC_FS_MAX_PACKET_SIZE);
  pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 1U;

  USBD_LL_OpenEP(pdev, CDC_OUT_EP, USBD_EP_TYPE_BULK, CDC_FS_MAX_PACKET_SIZE);
  pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 1U;

  USBD_LL_OpenEP(pdev, CDC_CMD_EP, USBD_EP_TYPE_INTR, 8);
  pdev->ep_in[CDC_CMD_EP & 0xFU].is_used = 1U;

  /* Init CDC line coding (115200-8-N-1) */
  hcomp->cdc.LineCoding[0] = 0x00;  /* baud rate low */
  hcomp->cdc.LineCoding[1] = 0xC2;  /* baud rate */
  hcomp->cdc.LineCoding[2] = 0x01;  /* baud rate */
  hcomp->cdc.LineCoding[3] = 0x00;  /* baud rate high */
  hcomp->cdc.LineCoding[4] = 0x00;  /* 1 stop bit */
  hcomp->cdc.LineCoding[5] = 0x00;  /* no parity */
  hcomp->cdc.LineCoding[6] = 0x08;  /* 8 data bits */
  hcomp->cdc.TxState = 0U;
  hcomp->cdc.RxState = 0U;

  /* Prepare CDC OUT for first reception */
  USBD_LL_PrepareReceive(pdev, CDC_OUT_EP, (uint8_t *)hcomp->cdc.data,
                         CDC_FS_MAX_PACKET_SIZE);

  /* Cache CDC handle for callback-safe transmit */
  CDC_CacheHandleDirect(&hcomp->cdc);

  /* Restore pClassData/pUserData */
  pdev->pClassData = saved_data;
  pdev->pUserData = saved_user;

  return USBD_OK;
}

/**
  * @brief  Composite_DeInit
  *         DeInit both MSC and CDC sub-classes
  * @param  pdev: device instance
  * @param  cfgidx: configuration index
  * @retval status
  */
static uint8_t Composite_DeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
  USBD_Composite_HandleTypeDef *hcomp;
  void *saved;

  hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;
  if (hcomp == NULL)
  {
    return USBD_FAIL;
  }

  saved = pdev->pClassData;

  /* DeInit MSC */
  pdev->pClassData = (void *)&hcomp->msc;
  pdev->pUserData = (void *)hcomp->storage_fops;
  USBD_LL_CloseEP(pdev, MSC_EPOUT_ADDR);
  pdev->ep_out[MSC_EPOUT_ADDR & 0xFU].is_used = 0U;
  USBD_LL_CloseEP(pdev, MSC_EPIN_ADDR);
  pdev->ep_in[MSC_EPIN_ADDR & 0xFU].is_used = 0U;

  /* DeInit CDC */
  pdev->pClassData = (void *)&hcomp->cdc;
  USBD_LL_CloseEP(pdev, CDC_IN_EP);
  pdev->ep_in[CDC_IN_EP & 0xFU].is_used = 0U;
  USBD_LL_CloseEP(pdev, CDC_OUT_EP);
  pdev->ep_out[CDC_OUT_EP & 0xFU].is_used = 0U;
  USBD_LL_CloseEP(pdev, CDC_CMD_EP);
  pdev->ep_in[CDC_CMD_EP & 0xFU].is_used = 0U;

  /* Free resources — free the actual composite handle */
  if (saved != NULL)
  {
    USBD_free(saved);
  }
  pdev->pClassData = NULL;
  return USBD_OK;
}

/**
  * @brief  Composite_Setup
  *         Dispatch setup requests to MSC or CDC based on interface
  * @param  pdev: device instance
  * @param  req: USB request
  * @retval status
  */
static uint8_t Composite_Setup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
  USBD_Composite_HandleTypeDef *hcomp;
  uint8_t ifnum;
  uint8_t ret = USBD_OK;
  void *saved_data;
  void *saved_user;

  hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;
  if (hcomp == NULL)
  {
    return USBD_FAIL;
  }

  /* Determine interface number from request */
  if ((req->bmRequest & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_INTERFACE)
  {
    ifnum = (uint8_t)(req->wIndex);
  }
  else if ((req->bmRequest & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_ENDPOINT)
  {
    /* For endpoint requests, map endpoint to interface */
    uint8_t ep = (uint8_t)(req->wIndex) & 0x7FU;
    if (ep == 0x01)
      ifnum = COMPOSITE_MSC_INTERFACE;
    else if (ep == 0x02 || ep == 0x03)
      ifnum = COMPOSITE_CDC_DATA_INTERFACE;
    else
      ifnum = 0xFF;
  }
  else
  {
    ifnum = 0xFF;
  }

  saved_data = pdev->pClassData;
  saved_user = pdev->pUserData;

  if (ifnum == COMPOSITE_MSC_INTERFACE)
  {
    /* Dispatch to MSC */
    pdev->pClassData = (void *)&hcomp->msc;
    pdev->pUserData = (void *)hcomp->storage_fops;
    ret = USBD_MSC_Setup(pdev, req);
  }
  else if ((ifnum == COMPOSITE_CDC_COMM_INTERFACE) ||
           (ifnum == COMPOSITE_CDC_DATA_INTERFACE))
  {
    /* Dispatch to CDC */
    pdev->pClassData = (void *)&hcomp->cdc;
    pdev->pUserData = (void *)hcomp->cdc_fops;
    ret = USBD_CDC_Setup(pdev, req);
  }
  else
  {
    /* Not our interface - pass through to core */
    if ((req->bmRequest & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_STANDARD)
    {
      switch (req->bRequest)
      {
        case USB_REQ_GET_INTERFACE:
        {
          uint8_t ifalt = 0U;
          USBD_CtlSendData(pdev, (uint8_t *)&ifalt, 1U);
          break;
        }
        case USB_REQ_SET_INTERFACE:
          break;
        case USB_REQ_GET_STATUS:
        {
          uint16_t status_info = 0U;
          USBD_CtlSendData(pdev, (uint8_t *)&status_info, 2U);
          break;
        }
        default:
          USBD_CtlError(pdev, req);
          ret = USBD_FAIL;
          break;
      }
    }
    else
    {
      USBD_CtlError(pdev, req);
      ret = USBD_FAIL;
    }
  }

  pdev->pClassData = saved_data;
  pdev->pUserData = saved_user;

  return ret;
}

/**
  * @brief  Composite_DataIn
  *         Dispatch DataIn to MSC or CDC based on endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */
static uint8_t Composite_DataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_Composite_HandleTypeDef *hcomp;
  uint8_t ret = USBD_OK;
  void *saved_data;
  void *saved_user;

  hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;
  if (hcomp == NULL)
  {
    return USBD_FAIL;
  }

  saved_data = pdev->pClassData;
  saved_user = pdev->pUserData;

  if (epnum == (MSC_EPIN_ADDR & 0x7FU))
  {
    /* MSC DataIn */
    pdev->pClassData = (void *)&hcomp->msc;
    pdev->pUserData = (void *)hcomp->storage_fops;
    ret = USBD_MSC_DataIn(pdev, epnum);
  }
  else if ((epnum == (CDC_IN_EP & 0x7FU)) ||
           (epnum == (CDC_CMD_EP & 0x7FU)))
  {
    /* CDC DataIn (bulk or notification) */
    pdev->pClassData = (void *)&hcomp->cdc;
    pdev->pUserData = (void *)hcomp->cdc_fops;
    ret = USBD_CDC_DataIn(pdev, epnum);
  }

  pdev->pClassData = saved_data;
  pdev->pUserData = saved_user;

  return ret;
}

/**
  * @brief  Composite_DataOut
  *         Dispatch DataOut to MSC or CDC based on endpoint
  * @param  pdev: device instance
  * @param  epnum: endpoint number
  * @retval status
  */

static uint8_t Composite_DataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
  USBD_Composite_HandleTypeDef *hcomp;
  uint8_t ret = USBD_OK;
  void *saved_data;
  void *saved_user;
  hcomp = (USBD_Composite_HandleTypeDef *)pdev->pClassData;
  if (hcomp == NULL)
  {
    return USBD_FAIL;
  }

  saved_data = pdev->pClassData;
  saved_user = pdev->pUserData;

  if (epnum == (MSC_EPOUT_ADDR & 0x7FU))
  {
    /* MSC DataOut */
    pdev->pClassData = (void *)&hcomp->msc;
    pdev->pUserData = (void *)hcomp->storage_fops;
    ret = USBD_MSC_DataOut(pdev, epnum);
  }
  else if (epnum == (CDC_OUT_EP & 0x7FU))
  {
    /* CDC DataOut */
    pdev->pClassData = (void *)&hcomp->cdc;
    pdev->pUserData = (void *)hcomp->cdc_fops;
    ret = USBD_CDC_DataOut(pdev, epnum);
  }

  pdev->pClassData = saved_data;
  pdev->pUserData = saved_user;

  return ret;
}

/**
  * @brief  Composite_GetHSCfgDesc
  *         Return HS configuration descriptor
  * @param  length: pointer to length
  * @retval pointer to descriptor
  */
static uint8_t *Composite_GetHSCfgDesc(uint16_t *length)
{
  *length = sizeof(USBD_Composite_CfgHSDesc);
  return USBD_Composite_CfgHSDesc;
}

/**
  * @brief  Composite_GetFSCfgDesc
  *         Return FS configuration descriptor
  * @param  length: pointer to length
  * @retval pointer to descriptor
  */
static uint8_t *Composite_GetFSCfgDesc(uint16_t *length)
{
  *length = sizeof(USBD_Composite_CfgFSDesc);
  return USBD_Composite_CfgFSDesc;
}

/**
  * @brief  Composite_GetOtherSpeedCfgDesc
  * @param  length: pointer to length
  * @retval pointer to descriptor
  */
static uint8_t *Composite_GetOtherSpeedCfgDesc(uint16_t *length)
{
  *length = sizeof(USBD_Composite_OtherSpeedCfgDesc);
  return USBD_Composite_OtherSpeedCfgDesc;
}

/**
  * @brief  Composite_GetDeviceQualifierDescriptor
  * @param  length: pointer to length
  * @retval pointer to descriptor
  */
static uint8_t *Composite_GetDeviceQualifierDescriptor(uint16_t *length)
{
  *length = sizeof(USBD_Composite_DeviceQualifierDesc);
  return USBD_Composite_DeviceQualifierDesc;
}

/**
  * @brief  USBD_Composite_RegisterStorage
  *         Register storage callbacks
  * @param  pdev: device instance
  * @param  fops: storage callbacks
  * @retval status
  */
uint8_t USBD_Composite_RegisterStorage(USBD_HandleTypeDef *pdev,
                                        USBD_StorageTypeDef *fops)
{
  UNUSED(pdev);
  g_comp_storage_fops = fops;
  return USBD_OK;
}

/**
  * @brief  USBD_Composite_RegisterCDC
  *         Register CDC interface callbacks
  * @param  pdev: device instance
  * @param  fops: CDC interface callbacks
  * @retval status
  */
uint8_t USBD_Composite_RegisterCDC(USBD_HandleTypeDef *pdev,
                                    USBD_CDC_ItfTypeDef *fops)
{
  UNUSED(pdev);
  g_comp_cdc_fops = fops;
  return USBD_OK;
}

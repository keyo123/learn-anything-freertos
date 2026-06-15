/**
  ******************************************************************************
  * @file    Project/STM32F10x_StdPeriph_Template/stm32f10x_it.c
  * @author  MCD Application Team
  * @version V3.5.0
  * @date    08-April-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTI

  AL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x_it.h"

#include "FreeRTOS.h" //FreeRTOSʹ��
#include "task.h"
#include "semphr.h"   /* xSemaphoreGiveFromISR */
#include "bsp_exti.h"

/** @addtogroup STM32F10x_StdPeriph_Template
 * @{
 */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* LED1任务句柄，在main.c中定义 */
extern TaskHandle_t LED1_Task_Handle;

/* 二值信号量句柄，在 demo_semphr.c 中定义 — ISR → 任务同步 */
extern SemaphoreHandle_t xBinarySem_Key1;
extern SemaphoreHandle_t xBinarySem_Key2;
extern TaskHandle_t xBtnNotifyTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
 * @brief  This function handles NMI exception.
 * @param  None
 * @retval None
 */
void NMI_Handler(void)
{
}

/**
 * @brief  This function handles Hard Fault exception.
 * @param  None
 * @retval None
 */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
 * @brief  This function handles Memory Manage exception.
 * @param  None
 * @retval None
 */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
 * @brief  This function handles Bus Fault exception.
 * @param  None
 * @retval None
 */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
 * @brief  This function handles Usage Fault exception.
 * @param  None
 * @retval None
 */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
 * @brief  This function handles SVCall exception.
 * @param  None
 * @retval None
 */
// void SVC_Handler(void)
//{
// }

/**
 * @brief  This function handles Debug Monitor exception.
 * @param  None
 * @retval None
 */
void DebugMon_Handler(void)
{
}

/**
 * @brief  This function handles PendSVC exception.
 * @param  None
 * @retval None
 */
// void PendSV_Handler(void)
//{
// }

///**
//  * @brief  This function handles SysTick Handler.
//  * @param  None
//  * @retval None
//  */
extern void xPortSysTickHandler(void);
// systick�жϷ�����
void SysTick_Handler(void)
{
#if (INCLUDE_xTaskGetSchedulerState == 1)
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
  {
#endif /* INCLUDE_xTaskGetSchedulerState */
    xPortSysTickHandler();
#if (INCLUDE_xTaskGetSchedulerState == 1)
  }
#endif /* INCLUDE_xTaskGetSchedulerState */
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/* FreeRTOS LED任务 — 按键中断处理 */

/**
  * @brief  KEY2按键中断(PC0)，二值信号量同步 or 恢复LED1_Task
  * @param  None
  * @retval None
  *
  * demo_semphr.c 运行时：xBinarySem_Key2 非 NULL → GiveFromISR 通知任务（Pipeline）
  * main.c        运行时：xBinarySem_Key2 == NULL → xTaskResumeFromISR（旧行为）
  */
void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(KEY2_INT_EXTI_LINE) == SET)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        if (xBinarySem_Key2 != NULL)
        {
            /* 练习 7：二值信号量 ISR 同步模式 */
            xSemaphoreGiveFromISR(xBinarySem_Key2, &xHigherPriorityTaskWoken);
        }
        else
        {
            /* 旧模式：恢复 LED1_Task（兼容 main.c） */
            xHigherPriorityTaskWoken = xTaskResumeFromISR(LED1_Task_Handle);
        }

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        EXTI_ClearITPendingBit(KEY2_INT_EXTI_LINE);
    }
}

/**
  * @brief  KEY1按键中断(PB11)，二值信号量同步 or 挂起LED1_Task
  * @param  None
  * @retval None
  *
  * demo_semphr.c 运行时：xBinarySem_Key1 非 NULL → GiveFromISR 通知任务
  * main.c        运行时：xBinarySem_Key1 == NULL → vTaskSuspend（旧行为）
  */
void EXTI15_10_IRQHandler(void)
{
    if (EXTI_GetITStatus(KEY1_INT_EXTI_LINE) == SET)
    {
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;

        if(xBtnNotifyTaskHandle != NULL)
        {
            vTaskNotifyGiveFromISR(xBtnNotifyTaskHandle,&xHigherPriorityTaskWoken);
        }
        else if (xBinarySem_Key1 != NULL)
        {
            /* 练习 7：二值信号量 ISR 同步模式 */
            xSemaphoreGiveFromISR(xBinarySem_Key1, &xHigherPriorityTaskWoken);
        }
        else
        {
            /* 旧模式：直接挂起 LED1_Task（兼容 main.c） */
            vTaskSuspend(LED1_Task_Handle);
        }

        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        EXTI_ClearITPendingBit(KEY1_INT_EXTI_LINE);
    }
}

/******************************************************************************/
/*                 STM32F10x Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f10x_xx.s).                                            */
/******************************************************************************/

/**
 * @}
 */

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/

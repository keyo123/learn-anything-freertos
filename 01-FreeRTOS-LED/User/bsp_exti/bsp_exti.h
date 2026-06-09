#ifndef __EXTI_H
#define	__EXTI_H


#include "stm32f10x.h"


// 引脚定义 —— 与 bsp_key.h 一致
#define KEY1_INT_GPIO_PORT         GPIOB
#define KEY1_INT_GPIO_CLK          (RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO)
#define KEY1_INT_GPIO_PIN          GPIO_Pin_11
#define KEY1_INT_EXTI_PORTSOURCE   GPIO_PortSourceGPIOB
#define KEY1_INT_EXTI_PINSOURCE    GPIO_PinSource11
#define KEY1_INT_EXTI_LINE         EXTI_Line11
#define KEY1_INT_EXTI_IRQ          EXTI15_10_IRQn

#define KEY1_IRQHandler            EXTI15_10_IRQHandler


#define KEY2_INT_GPIO_PORT         GPIOC
#define KEY2_INT_GPIO_CLK          (RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO)
#define KEY2_INT_GPIO_PIN          GPIO_Pin_0
#define KEY2_INT_EXTI_PORTSOURCE   GPIO_PortSourceGPIOC
#define KEY2_INT_EXTI_PINSOURCE    GPIO_PinSource0
#define KEY2_INT_EXTI_LINE         EXTI_Line0
#define KEY2_INT_EXTI_IRQ          EXTI0_IRQn

#define KEY2_IRQHandler            EXTI0_IRQHandler

void EXTI_Key_Config(void);


#endif /* __EXTI_H */

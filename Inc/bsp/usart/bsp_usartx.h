#ifndef __BSP_USARTX_H__
#define __BSP_USARTX_H__

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <stdio.h>


/* 类型定义 ------------------------------------------------------------------*/
/* 宏定义 --------------------------------------------------------------------*/
#define USARTx                                 UART4
#define USARTx_BAUDRATE                        115200
#define USART_RCC_CLK_ENABLE()                 __HAL_RCC_UART4_CLK_ENABLE()
#define USART_RCC_CLK_DISABLE()                __HAL_RCC_UART4_CLK_DISABLE()

#define USARTx_GPIO_ClK_ENABLE()               __HAL_RCC_GPIOC_CLK_ENABLE()
#define USARTx_Tx_GPIO_PIN                     GPIO_PIN_10
#define USARTx_Tx_GPIO                         GPIOC
#define USARTx_Rx_GPIO_PIN                     GPIO_PIN_11   
#define USARTx_Rx_GPIO                         GPIOC

#define USARTx_AFx                             GPIO_AF8_UART4


#define USARTx_IRQHANDLER                      UART4_IRQHandler
#define USARTx_IRQn                            UART4_IRQn


/* 扩展变量 ------------------------------------------------------------------*/
extern UART_HandleTypeDef husartx;

/* 函数声明 ------------------------------------------------------------------*/
void MX_USARTx_Init(void);


#endif  /* __BSP_USARTX_H__ */

/******************* (C) COPYRIGHT 2015-2020 硬石嵌入式开发团队 *****END OF FILE****/

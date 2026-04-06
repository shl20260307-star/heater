//#ifndef __DAC_H__
//#define	__DAC_H__

#ifndef _bsp_dac_H
#define _bsp_dac_H

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* 类型定义 ------------------------------------------------------------------*/
/* 宏定义 --------------------------------------------------------------------*/
#define DACx                            DAC
#define DACx_CHANNEL_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOA_CLK_ENABLE()

#define DACx_CLK_ENABLE()               __HAL_RCC_DAC_CLK_ENABLE()
#define DACx_FORCE_RESET()              __HAL_RCC_DAC_FORCE_RESET()
#define DACx_RELEASE_RESET()            __HAL_RCC_DAC_RELEASE_RESET()

/* 定义为通道1的引脚 PA4 */
#define DACx_CHANNEL1_PIN               GPIO_PIN_4
#define DACx_CHANNEL1_GPIO_PORT         GPIOA
/* 定义为 DAC 通道1 */
#define DACx_CHANNEL1                   DAC_CHANNEL_1

/* 定义DAC通道引脚，这里使用 PA5 （DAC2）*/
#define DACx_CHANNEL2_PIN               GPIO_PIN_5
#define DACx_CHANNEL2_GPIO_PORT         GPIOA
/* 定义DAC通道，这里使用通道2 */
#define DACx_CHANNEL2                   DAC_CHANNEL_2


///* 定义DAC通道引脚，这里使用 PA5 （DAC2）*/
//#define DACx_CHANNEL2_PIN               GPIO_PIN_4
//#define DACx_CHANNEL2_GPIO_PORT         GPIOA
//
///* 定义DAC通道，这里使用通道2 */
//#define DACx_CHANNEL2                   DAC_CHANNEL_1


/* 变量声明 ------------------------------------------------------------------*/
extern DAC_HandleTypeDef hdac;
/* 函数声明 ------------------------------------------------------------------*/
void MX_DAC_Init(void);

#endif /* __DAC_H */

/******************* (C) COPYRIGHT 2015-2020 硬石嵌入式开发团队 *****END OF FILE****/

#ifndef __ADC_DMA_H
#define __ADC_DMA_H

#include "stm32f4xx_hal.h"      // 包含所有外设驱动，包括 DMA
#include "stm32f4xx_hal_dma.h"  // 确保 DMA_CHANNEL_0 等宏可用

#define SAMPLES_COUNT 100
#define ADC_DMA_BUF_LEN SAMPLES_COUNT

// DMA 句柄在 .c 中定义
extern DMA_HandleTypeDef hdma_adc;

// DMA 缓冲区，在 main.c 或模块中定义
extern uint16_t adc_dma_buffer[ADC_DMA_BUF_LEN];
extern TIM_HandleTypeDef htim2;


// 初始化 DMA
void DMA_Init(void);

// 启动 ADC + DMA 传输
void ADC_DMA_Start(uint16_t *buf, uint32_t len);

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc);
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc);

#endif /* __ADC_DMA_H */

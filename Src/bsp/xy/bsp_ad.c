#include "bsp_ad.h"
#include "stm32f4xx_hal.h"
/* 私有类型定义 --------------------------------------------------------------*/
/* 私有宏定义 ----------------------------------------------------------------*/
/* 私有变量 ------------------------------------------------------------------*/
ADC_HandleTypeDef hadcx;

/* 扩展变量 ------------------------------------------------------------------*/
/* 私有函数原形 --------------------------------------------------------------*/
/* 函数体 --------------------------------------------------------------------*/
/**
  * 函数功能: AD转换初始化
  * 输入参数: 无
  * 返 回 值: 无
  * 说    明：无
  */
void MX_ADCx_Init(void)
{
  ADC_ChannelConfTypeDef sConfig;

  hadcx.Instance = ADCx;
  hadcx.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadcx.Init.Resolution = ADC_RESOLUTION_12B;
  hadcx.Init.ScanConvMode = DISABLE;
 
  /*保证 ADC 每次收到 TIM2 的 TRGO 信号后由硬件自动启动，并将 N 个采样结果通过 DMA 存入 adc_dma_buffer[] 数组 */
  hadcx.Init.ContinuousConvMode    = DISABLE;  // 由定时器触发，不自动连续 
  hadcx.Init.DiscontinuousConvMode = DISABLE;
 // hadcx.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  /*在 TIM2 的 TRGO 上升沿触发 ADC 开始一次采样/转换。也就是说ADC 的采样频率 = TIM2 TRGO 频率。*/
  hadcx.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING; //上升沿触发
  //hadcx.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadcx.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO; // 触发源选TIM2 TRGO
  hadcx.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadcx.Init.NbrOfConversion = 1;//转换通道数量为1
  //hadcx.Init.DMAContinuousRequests = DISABLE;//此处不启用 DMA
  hadcx.Init.DMAContinuousRequests = ENABLE; //让 ADC 连续产生 DMA 请求 
  hadcx.Init.EOCSelection = ADC_EOC_SINGLE_CONV;//EOC（End Of Conversion）标志选择为单次转换
  
  
  HAL_ADC_Init(&hadcx);
  
  /* 配置采样通道 */
  sConfig.Channel = ADC_CHANNEL;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_56CYCLES;  //采样时间为 56 个 ADC 时钟周期
  HAL_ADC_ConfigChannel(&hadcx, &sConfig);
}

/**
  * 函数功能: ADC外设初始化配置
  * 输入参数: hadc：AD外设句柄类型指针
  * 返 回 值: 无
  * 说    明: 该函数被HAL库内部调用
  */
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
  GPIO_InitTypeDef GPIO_InitStruct;
  if(hadc->Instance==ADCx)
  {
    /* 外设时钟使能 */
    ADCx_RCC_CLK_ENABLE();
    
    /* AD转换通道引脚时钟使能 */
    ADC_GPIO_ClK_ENABLE();
    
    /* AD转换通道引脚初始化 */
    GPIO_InitStruct.Pin = ADC_GPIO_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ADC_GPIO, &GPIO_InitStruct);

    /* 外设中断优先级配置和使能中断 */
    HAL_NVIC_SetPriority(ADC_IRQn, 2, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
  }
}

/**
  * 函数功能: ADC外设反初始化配置
  * 输入参数: hadc：AD外设句柄类型指针
  * 返 回 值: 无
  * 说    明: 该函数被HAL库内部调用
  */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{
  if(hadc->Instance==ADCx)
  {
    /* 关闭ADC外设时钟 */
    ADCx_RCC_CLK_DISABLE();
  
    /* AD转换通道引脚反初始化 */
    HAL_GPIO_DeInit(ADC_GPIO, ADC_GPIO_PIN);

    /* 禁用外设中断 */
    HAL_NVIC_DisableIRQ(ADC_IRQn);

  }
} 

/******************* (C) COPYRIGHT 2015-2020 硬石嵌入式开发团队 *****END OF FILE****/

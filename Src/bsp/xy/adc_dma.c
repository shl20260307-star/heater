#include "adc_dma.h"
#include "bsp_ad.h"
DMA_HandleTypeDef hdma_adc;           // 新增 DMA 句柄

uint16_t adc_dma_buffer[ADC_DMA_BUF_LEN];  // 全局缓冲区定义
TIM_HandleTypeDef htim2;   // ? 全局

/*
工作流程:
TIM2 以 TRGO（Update 事件）触发 ADC 每次开始一次转换；
ADC 在每次转换完成后通过 DMA 把 12bit 的结果以 16-bit 半字（uint16_t）写入内存缓冲区（循环模式）。
DMA 可以在半满和满时触发中断，方便上层处理或拷贝数据。
TIM2 周期 = 1 ms（1 kHz）
→ 每 1 ms，TRGO 输出一个脉冲
→ ADC 收到触发，采集一次数据
→ 保证采样频率稳定在 1 kHz
*/
// 1. 配置 TIM2 产生 1 kHz TRGO
void TIM2_Init(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();    // 使能 TIM2 时钟 

    //TIM_HandleTypeDef htim2;
    htim2.Instance               = TIM2;
    htim2.Init.Prescaler = 83;// 分频寄存器值。采样频率改为1khz,APB1分频系数大于 1（即不等于 1），定时器时钟 = APBx_CLK × 2=42*2=84，
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 999; // 1MHz/(999+1)=1kHz
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    HAL_TIM_Base_Init(&htim2);

    TIM_MasterConfigTypeDef mcfg = {
        .MasterOutputTrigger = TIM_TRGO_UPDATE,     
        .MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE
    };// 把 TIM2 的 TRGO 设为“Update event”（即每次计数到 ARR 发生更新时，产生 TRGO）
    HAL_TIMEx_MasterConfigSynchronization(&htim2, &mcfg);
    HAL_TIM_Base_Start(&htim2);
    
}

// 2. 配置  DMA
void DMA_Init(void)
{
    

    // 2.1 时钟使能
    __HAL_RCC_DMA2_CLK_ENABLE();    // DMA2 时钟 

   
    // 2.2 配置 DMA2_Stream0, Channel2，循环模式
    hdma_adc.Instance                 = DMA2_Stream0;
    hdma_adc.Init.Channel             = DMA_CHANNEL_2;
    hdma_adc.Init.Direction           = DMA_PERIPH_TO_MEMORY;
    hdma_adc.Init.PeriphInc           = DMA_PINC_DISABLE;
    hdma_adc.Init.MemInc              = DMA_MINC_ENABLE;
    hdma_adc.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc.Init.MemDataAlignment    = DMA_MDATAALIGN_HALFWORD;
    hdma_adc.Init.Mode                = DMA_CIRCULAR;
    hdma_adc.Init.Priority            = DMA_PRIORITY_HIGH;
   
    HAL_DMA_Init(&hdma_adc);

    __HAL_LINKDMA(&hadcx, DMA_Handle, hdma_adc);


     // 使能半传输和全传输完成中断，即HAL_ADC_ConvHalfCpltCallback和HAL_ADC_ConvCpltCallback
     __HAL_DMA_ENABLE_IT(&hdma_adc, DMA_IT_HT | DMA_IT_TC);    // DMA 完成或半完成时会触发中断
 
     
    /* 配置 DMA 中断 .这样，HAL_DMA_IRQHandler(&hdma_adc) 才会被调用，从而触发 HAL_ADC_ConvHalfCpltCallback
     在 stm32f4xx_it.c 的对应 IRQHandler 中调用 HAL_DMA_IRQHandler(&hdma_adc)*/
     // 在 NVIC 中配置 DMA2_Stream0 中断优先级并使能
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 1, 0);            // 设为优先级 1，抢占优先级 1，子优先级 0
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);                   // 使能中断通道
    
}
void ADC_DMA_Start(uint16_t *buf, uint32_t len) {
    HAL_ADC_Start_DMA(&hadcx, (uint32_t*)buf, len);//长度 len 表示期望的转换次数（即缓冲区元素个数）,
    //当 DMA 在循环模式时，会一直在该缓冲区循环写入数据；当写到一半或满时，会触发前面使能的 HT/TC 中断回调。
}


#include "solution_intensity.h"
#include "bsp_ad.h"
#include "bsp_dac.h"
#include "adc_dma.h"
#include <math.h>


////#define LIGHT_LEVELS 10      //定义几种档位光强 1700-1800之间变换
////#define ROW_HOLES        12
#define ADC_SAMPLES 4000 //每个孔每种光强采集1000个点取平均值
//volatile uint32_t adc_sum = 0;     // 累加和
//volatile uint16_t adc_count = 0;   // 样本数
//volatile uint8_t  adc_done = 0;    // 采样完成标志
//static float light_table[LIGHT_LEVELS];   //① 光强表：只在本文件可见 
//float row_result[ROW_HOLES][LIGHT_LEVELS];  // row_result[孔索引][光强索引]
//
//
//#define ADC_TOTAL_POINTS 2000
//volatile uint32_t adc_count1 = 0;
//volatile double adc_sum1 = 0.0;
//volatile double adc_sq_sum = 0.0;
//volatile uint8_t adc_result_ready = 0;
//double adc_mean = 0.0;
//double adc_std = 0.0;


 volatile uint32_t adc_count = 0;
 volatile double adc_sum = 0.0;
 volatile double adc_sq_sum = 0.0;
 volatile uint8_t adc_done = 0;

double last_mean = 0.0;
double last_std  = 0.0;
float v1,v2;



static inline void ADC_Accumulate(uint16_t raw)
{
    /* ① 原始值立刻转浮点，避免整数溢出 */
    float v = raw * 2.5 / 4095.0;
    //v1=raw;
    //v2=v;

    adc_sum    += v;
    adc_sq_sum += v * v;
    adc_count++;

    if (adc_count >= ADC_SAMPLES)
    {
        double mean = adc_sum / ADC_SAMPLES;
        double var  = adc_sq_sum / ADC_SAMPLES - (mean * mean);

        /* ② 防浮点误差（非常重要） */
        if (var < 0)
            var = 0;

        last_mean = mean;
        last_std  = sqrt(var);   // sqrt(double)，和 last_std 类型一致

        adc_done = 1;

        /* ③ 复位，为下一轮准备 */
        adc_sum = 0.0;
        adc_sq_sum = 0.0;
        adc_count = 0;
    }
}


void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC3)
        for (int i = 0; i < SAMPLES_COUNT / 2; i++)
            ADC_Accumulate(adc_dma_buffer[i]);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC3)
        for (int i = SAMPLES_COUNT / 2; i < SAMPLES_COUNT; i++)
            ADC_Accumulate(adc_dma_buffer[i]);
}

//一次采样测试
void SolutionIntensity_StartSample(void)
{
   
    adc_done = 0;
    adc_sum = 0.0;
    adc_sq_sum = 0.0;
    adc_count = 0;
    
    //关键代码TIM2重新开启关闭
    HAL_TIM_Base_Stop(&htim2);   // ★ 先停 TIM2
    __HAL_TIM_SET_COUNTER(&htim2, 0);//CNT 强制拉回 0,全新的采集周期起点
    HAL_TIM_Base_Start(&htim2);  // ★ 重新对齐 TRGO

    //关键代码ADC_DMA重新开启关闭
    ADC_DMA_Start(adc_dma_buffer, SAMPLES_COUNT); 
    while (!adc_done); // 等待采样完成（此时中断在跑）
    HAL_ADC_Stop_DMA(&hadcx);//停止 ADC + DMA 
    __HAL_ADC_DISABLE(&hadcx);
 hadcx.Instance->CR2 &= ~ADC_CR2_DMA; // 清除 DMA 使能位

    
    /* ★ 注意这里要想两个值分开打印,必须要先转存再打印*/
    /*否则数据不准确，详情见每日小计2026.1.28 */
   // printf("%.6f  %.6f\r\n",last_mean, last_std);//直接打印两个数值，是准确的
    double mean_local=last_mean;
    double std_local=last_std;
    
    printf("%.6f\r\n", mean_local);
    printf("%.6f\r\n", std_local);
    printf("\r\n");   // ★ 空一行
}



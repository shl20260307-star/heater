#include "bsp_Limitsensor.h"

/*******************************************************************************
* 函 数 名         : Limitsensor_Init
* 函数功能	   : Limitsensor初始化函数
* 输    入         : 无
* 输    出         : 无
*******************************************************************************/
void Limitsensor_Init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0}; // 定义结构体变量

    // 使能X轴端口时钟
    __HAL_RCC_GPIOC_CLK_ENABLE();
    //使能滤光片端口时钟
    __HAL_RCC_GPIOH_CLK_ENABLE(); 
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
 

    // 配置X、Y轴GPIO参数
    GPIO_InitStruct.Pin = Limitsensor_PIN_X|Limitsensor_PIN_Y; // 管脚设置
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;  // 设为输入模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;      // 上拉
    HAL_GPIO_Init(Limitsensor_PORT, &GPIO_InitStruct); // 初始化X轴结构体
  
    // 配置hall参数
    GPIO_InitStruct.Pin = Hallsensor_PIN_hall; // 管脚设置
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;  // 设为输入模式
    GPIO_InitStruct.Pull = GPIO_PULLUP;      // 上拉
    HAL_GPIO_Init(Hallsensor_PORT , &GPIO_InitStruct); // 初始化结构体  

}
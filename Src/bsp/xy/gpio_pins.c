/* gpio_pins.c  --- 新增文件 */
#include "gpio_pins.h"

void MX_GPIO_PE_PI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOI_CLK_ENABLE();

    /* PE0 输出 */
    GPIO_InitStruct.Pin = GPIO_PE0_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_PE0_PORT, &GPIO_InitStruct);

    /* PI6 输出 */
    GPIO_InitStruct.Pin = GPIO_PI6_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIO_PI6_PORT, &GPIO_InitStruct);
    

    /* 初始全部清零（低电平）*/
    HAL_GPIO_WritePin(GPIO_PE0_PORT, GPIO_PE0_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIO_PI6_PORT, GPIO_PI6_PIN, GPIO_PIN_RESET);
}
//把指定的引脚电平翻转一次（高变低、低变高）
void Toggle_PE0(void) { HAL_GPIO_TogglePin(GPIO_PE0_PORT, GPIO_PE0_PIN); }//HAL 库里的引脚翻转函数
void Toggle_PI6(void) { HAL_GPIO_TogglePin(GPIO_PI6_PORT, GPIO_PI6_PIN); }

/* 返回 1 或 0（当前电平） */
uint8_t Toggle_PE0_and_GetState(void)
{
    //HAL_GPIO_TogglePin(GPIO_PE0_PORT, GPIO_PE0_PIN);
    return (HAL_GPIO_ReadPin(GPIO_PE0_PORT, GPIO_PE0_PIN) == GPIO_PIN_SET) ? 1U : 0U;
}

uint8_t Toggle_PI6_and_GetState(void)
{
    //HAL_GPIO_TogglePin(GPIO_PI6_PORT, GPIO_PI6_PIN);
    return (HAL_GPIO_ReadPin(GPIO_PI6_PORT, GPIO_PI6_PIN) == GPIO_PIN_SET) ? 1U : 0U;
}


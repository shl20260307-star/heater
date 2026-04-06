/* gpio_pins.h  --- 新增文件 */
#ifndef GPIO_PINS_H
#define GPIO_PINS_H

#include "stm32f4xx_hal.h"

/* 引脚定义：PE0(169), PI6(175), PI7(176) */
#define GPIO_PE0_PORT    GPIOE
#define GPIO_PE0_PIN     GPIO_PIN_0

#define GPIO_PI6_PORT    GPIOI
#define GPIO_PI6_PIN     GPIO_PIN_6

#define GPIO_PI7_PORT    GPIOI
#define GPIO_PI7_PIN     GPIO_PIN_7

/* 初始化函数，在 main() 初始化序列中调用 */
void MX_GPIO_PE_PI_Init(void);

/* 翻转操作 */
void Toggle_PE0(void);
void Toggle_PI6(void);



/* 翻转对应引脚并返回该引脚当前电平（1 = SET, 0 = RESET） */
uint8_t Toggle_PE0_and_GetState(void);
uint8_t Toggle_PI6_and_GetState(void);
uint8_t Toggle_PI7_and_GetState(void);

/* 翻转由 mask 指定的引脚（bit0->PE0, bit1->PI6, bit2->PI7），
   返回当前三位状态掩码（bit0..bit2） */
uint8_t Toggle_Mask_and_GetStatus(uint8_t mask);

#endif /* GPIO_PINS_H */

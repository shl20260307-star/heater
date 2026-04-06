#ifndef _bsp_Limitsensor_H
#define _bsp_Limitsensor_H

#include "stm32f4xx_hal.h"

/* LimitsensorÒý½Å¶¨Òå */

#define Limitsensor_PORT  GPIOE
#define Limitsensor_PIN_X   GPIO_PIN_11
#define Limitsensor_PIN_Y   GPIO_PIN_15

#define Hallsensor_PORT      GPIOG
#define Hallsensor_PIN_hall  GPIO_PIN_8
void Limitsensor_Init(void);

#endif
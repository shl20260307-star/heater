#ifndef __DS18B20_MULTI_H
#define __DS18B20_MULTI_H

#include "stm32f4xx_hal.h"

void DS18B20_GPIOs_Init(void);
uint8_t DS18B20_StartConversion_PortPin(GPIO_TypeDef *port, uint16_t pin);
float DS18B20_ReadTemperature_PortPin(GPIO_TypeDef *port, uint16_t pin);

#endif
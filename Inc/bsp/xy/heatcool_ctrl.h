#ifndef __HEATCOOL_CTRL_H
#define __HEATCOOL_CTRL_H

#include "stm32f4xx_hal.h"

typedef enum {
    HEATCOOL_MODE_OFF = 0,
    HEATCOOL_MODE_HEAT = 1,
    HEATCOOL_MODE_COOL = 2
} HeatCoolMode_t;

void HeatCool_Init(void);
void HeatCool_Enable(uint8_t enable);
uint8_t HeatCool_IsEnabled(void);

void HeatCool_SetTargetC(float target_c);
float HeatCool_GetTargetC(void);

void HeatCool_SetKp(float kp);

void HeatCool_SetMode(HeatCoolMode_t mode);
HeatCoolMode_t HeatCool_GetMode(void);

void HeatCool_SetPowerRaw(uint8_t value);
uint8_t HeatCool_GetPowerRaw(void);

void HeatCool_ControlStep(float current_temp_c);

void MCP41010_write(uint8_t value);
void HEAT(void);
void COOL(void);

#endif
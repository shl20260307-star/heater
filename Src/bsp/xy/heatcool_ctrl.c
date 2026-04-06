#include "heatcool_ctrl.h"
#include <math.h>

static SPI_HandleTypeDef hspi2;

static uint8_t g_enabled = 0;
static float g_target_temp_c = 25.0f;
static float g_kp = 8.0f;
static uint8_t g_power_raw = 0;
static HeatCoolMode_t g_mode = HEATCOOL_MODE_OFF;

static void HeatCool_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);
}

static void MCP41010_Bus_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOI_CLK_ENABLE();
    __HAL_RCC_SPI2_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF5_SPI2;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_0;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_0, GPIO_PIN_SET);

    hspi2.Instance = SPI2;
    hspi2.Init.Mode = SPI_MODE_MASTER;
    hspi2.Init.Direction = SPI_DIRECTION_2LINES;
    hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
    hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
    hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
    hspi2.Init.NSS = SPI_NSS_SOFT;
    hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_64;
    hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
    hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
    hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    hspi2.Init.CRCPolynomial = 7;

    HAL_SPI_Init(&hspi2);
}

void MCP41010_write(uint8_t value)
{
    uint8_t tx[2];
    uint8_t out = value;

    if (out > 40U) {
        out = 40U;
    }

    tx[0] = 0x11U;
    tx[1] = out;

    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&hspi2, tx, 2, 100);
    HAL_GPIO_WritePin(GPIOI, GPIO_PIN_0, GPIO_PIN_SET);
}

void HEAT(void)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_SET);
}

void COOL(void)
{
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_SET);
    HAL_Delay(500);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_13, GPIO_PIN_RESET);
}

void HeatCool_Init(void)
{
    HeatCool_GPIO_Init();
    MCP41010_Bus_Init();
    HeatCool_SetPowerRaw(0);
    HeatCool_SetMode(HEATCOOL_MODE_OFF);
    g_enabled = 0;
}

void HeatCool_Enable(uint8_t enable)
{
    g_enabled = enable ? 1U : 0U;
    if (!g_enabled) {
        HeatCool_SetPowerRaw(0);
        HeatCool_SetMode(HEATCOOL_MODE_OFF);
    }
}

uint8_t HeatCool_IsEnabled(void)
{
    return g_enabled;
}

void HeatCool_SetTargetC(float target_c)
{
    g_target_temp_c = target_c;
}

float HeatCool_GetTargetC(void)
{
    return g_target_temp_c;
}

void HeatCool_SetKp(float kp)
{
    if (kp > 0.0f) {
        g_kp = kp;
    }
}

void HeatCool_SetMode(HeatCoolMode_t mode)
{
    if (mode == g_mode) {
        return;
    }

    if (mode == HEATCOOL_MODE_OFF) {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12, GPIO_PIN_RESET);
        g_mode = HEATCOOL_MODE_OFF;
        return;
    }

    if (mode == HEATCOOL_MODE_HEAT) {
        HEAT();
        g_mode = HEATCOOL_MODE_HEAT;
    } else if (mode == HEATCOOL_MODE_COOL) {
        COOL();
        g_mode = HEATCOOL_MODE_COOL;
    }
}

HeatCoolMode_t HeatCool_GetMode(void)
{
    return g_mode;
}

void HeatCool_SetPowerRaw(uint8_t value)
{
    uint8_t out = value;
    if (out > 40U) {
        out = 40U;
    }
    g_power_raw = out;
    MCP41010_write(out);
}

uint8_t HeatCool_GetPowerRaw(void)
{
    return g_power_raw;
}

void HeatCool_ControlStep(float current_temp_c)
{
    float err;
    float mag;
    HeatCoolMode_t next_mode;
    uint8_t pwr;

    if (!g_enabled) {
        HeatCool_SetPowerRaw(0);
        HeatCool_SetMode(HEATCOOL_MODE_OFF);
        return;
    }

    err = g_target_temp_c - current_temp_c;
    mag = fabsf(err);

    if (mag <= 0.2f) {
        HeatCool_SetPowerRaw(0);
        HeatCool_SetMode(HEATCOOL_MODE_OFF);
        return;
    }

    next_mode = (err > 0.0f) ? HEATCOOL_MODE_HEAT : HEATCOOL_MODE_COOL;
    pwr = (uint8_t)(g_kp * mag + 0.5f);
    if (pwr > 40U) {
        pwr = 40U;
    }

    HeatCool_SetMode(next_mode);
    HeatCool_SetPowerRaw(pwr);
}
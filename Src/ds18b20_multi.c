#include "ds18b20_multi.h"

extern void TIM6_DelayUs(uint16_t us);

static void DS18B20_PinAsOutput(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

static void DS18B20_PinAsInput(GPIO_TypeDef *port, uint16_t pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(port, &GPIO_InitStruct);
}

static void DS18B20_WriteLow(GPIO_TypeDef *port, uint16_t pin)
{
    DS18B20_PinAsOutput(port, pin);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_RESET);
}

static void DS18B20_ReleaseBus(GPIO_TypeDef *port, uint16_t pin)
{
    DS18B20_PinAsOutput(port, pin);
    HAL_GPIO_WritePin(port, pin, GPIO_PIN_SET);
}

static uint8_t DS18B20_ReadBus(GPIO_TypeDef *port, uint16_t pin)
{
    return (uint8_t)HAL_GPIO_ReadPin(port, pin);
}

static uint8_t DS18B20_IsBusIdleHigh(GPIO_TypeDef *port, uint16_t pin)
{
    DS18B20_ReleaseBus(port, pin);
    TIM6_DelayUs(8);
    DS18B20_PinAsInput(port, pin);
    TIM6_DelayUs(8);
    return (DS18B20_ReadBus(port, pin) == GPIO_PIN_SET) ? 1U : 0U;
}

static uint8_t DS18B20_Reset_PortPin(GPIO_TypeDef *port, uint16_t pin)
{
    uint8_t presence;

    if (!DS18B20_IsBusIdleHigh(port, pin)) {
        return 0;
    }

    DS18B20_WriteLow(port, pin);
    TIM6_DelayUs(480);
    DS18B20_ReleaseBus(port, pin);
    TIM6_DelayUs(8);
    DS18B20_PinAsInput(port, pin);
    TIM6_DelayUs(62);
    presence = (DS18B20_ReadBus(port, pin) == GPIO_PIN_RESET) ? 1U : 0U;
    TIM6_DelayUs(410);

    return presence;
}

static void DS18B20_WriteBit_PortPin(GPIO_TypeDef *port, uint16_t pin, uint8_t bit)
{
    DS18B20_WriteLow(port, pin);
    if (bit) {
        TIM6_DelayUs(6);
        DS18B20_ReleaseBus(port, pin);
        TIM6_DelayUs(64);
    } else {
        TIM6_DelayUs(60);
        DS18B20_ReleaseBus(port, pin);
        TIM6_DelayUs(10);
    }
}

static uint8_t DS18B20_ReadBit_PortPin(GPIO_TypeDef *port, uint16_t pin)
{
    uint8_t bit;

    DS18B20_WriteLow(port, pin);
    TIM6_DelayUs(6);
    DS18B20_PinAsInput(port, pin);
    TIM6_DelayUs(9);
    bit = DS18B20_ReadBus(port, pin);
    TIM6_DelayUs(55);

    return bit;
}

static void DS18B20_WriteByte_PortPin(GPIO_TypeDef *port, uint16_t pin, uint8_t data)
{
    uint8_t i;
    for (i = 0; i < 8; i++) {
        DS18B20_WriteBit_PortPin(port, pin, data & 0x01U);
        data >>= 1;
    }
}

static uint8_t DS18B20_ReadByte_PortPin(GPIO_TypeDef *port, uint16_t pin)
{
    uint8_t i;
    uint8_t data = 0;

    for (i = 0; i < 8; i++) {
        data >>= 1;
        if (DS18B20_ReadBit_PortPin(port, pin)) {
            data |= 0x80U;
        }
    }

    return data;
}

static uint8_t DS18B20_CRC8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    uint8_t i;
    uint8_t j;

    for (i = 0; i < len; i++) {
        uint8_t inbyte = data[i];
        for (j = 0; j < 8; j++) {
            uint8_t mix = (crc ^ inbyte) & 0x01U;
            crc >>= 1;
            if (mix) {
                crc ^= 0x8CU;
            }
            inbyte >>= 1;
        }
    }

    return crc;
}

void DS18B20_GPIOs_Init(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();

    DS18B20_ReleaseBus(GPIOD, GPIO_PIN_11);
    DS18B20_ReleaseBus(GPIOD, GPIO_PIN_10);
    DS18B20_ReleaseBus(GPIOD, GPIO_PIN_9);
    DS18B20_ReleaseBus(GPIOD, GPIO_PIN_8);
}

uint8_t DS18B20_StartConversion_PortPin(GPIO_TypeDef *port, uint16_t pin)
{
    if (!DS18B20_Reset_PortPin(port, pin)) {
        return 0;
    }

    DS18B20_WriteByte_PortPin(port, pin, 0xCCU);
    DS18B20_WriteByte_PortPin(port, pin, 0x44U);

    return 1;
}

float DS18B20_ReadTemperature_PortPin(GPIO_TypeDef *port, uint16_t pin)
{
    uint8_t scratchpad[9];
    uint8_t i;
    int16_t raw;

    if (!DS18B20_Reset_PortPin(port, pin)) {
        return -127.0f;
    }

    DS18B20_WriteByte_PortPin(port, pin, 0xCCU);
    DS18B20_WriteByte_PortPin(port, pin, 0xBEU);

    for (i = 0; i < 9; i++) {
        scratchpad[i] = DS18B20_ReadByte_PortPin(port, pin);
    }

    /* 配置寄存器只允许 9~12bit 分辨率的固定值，0x00/0xFF 常见于总线异常或无器件 */
    if (!(scratchpad[4] == 0x1FU || scratchpad[4] == 0x3FU ||
          scratchpad[4] == 0x5FU || scratchpad[4] == 0x7FU)) {
        return -125.0f;
    }

    if (DS18B20_CRC8(scratchpad, 8) != scratchpad[8]) {
        return -126.0f;
    }

    raw = (int16_t)(((uint16_t)scratchpad[1] << 8) | scratchpad[0]);
    return (float)raw / 16.0f;
}
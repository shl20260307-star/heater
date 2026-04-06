#ifndef __DS18B20_H
#define __DS18B20_H
#include "stm32f4xx_hal.h"
#include <stdint.h>
/*
  说明：此头文件为“**外接 VDD（非寄生供电）**”版本
  - 假设每个 DS18B20 的 VDD 接到稳定 3.3V 
  - DQ 线上应有外部 4.7k 上拉到 VCC
*/
//定义 DS18B20 连接到 PD12 引脚
#define DS18B20_PORT GPIOD
#define DS18B20_PIN GPIO_PIN_12
#define MAX_DS18B20 5 // 支持最多发现 8 个设备，使用时只取前5个即可

// 存放发现到的 ROM 号（每个 ROM 8 字节）
extern uint8_t ds18_roms[MAX_DS18B20][8];
extern uint8_t ds18_count; // 实际发现的设备数

// 搜索总线上的设备，返回发现数量（<= max_devices）
uint8_t DS18B20_SearchDevices(uint8_t max_devices);

// 用 Match ROM 按 ROM 单独读取 scratchpad 并返回温度或错误码（同 DS18B20_ReadTemperature，但带 ROM）
float DS18B20_ReadTemperature_ByROM(const uint8_t rom[8]);



uint8_t  DS18B20_Init(void);
float  DS18B20_ReadTemperature(void);//读取温度
uint8_t  DS18B20_Reset(void);//复位DS18B20（保持不变）
void  DS18B20_WriteByte(uint8_t dat);
uint8_t  DS18B20_ReadByte(void);

uint8_t DS18B20_ReadBit(void);
void    DS18B20_WriteBit(uint8_t bit);



#endif // __DS18B20_H

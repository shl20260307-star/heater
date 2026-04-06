#include "stm32f4xx_hal.h"
#include "main.h"
#include "ds18b20.h"

// 全局 ROM 列表
uint8_t ds18_roms[MAX_DS18B20][8];
uint8_t ds18_count = 0;


// 引脚输出模式配置
void DS18B20_GPIO_Output(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_PIN;       
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;  // 开漏输出（符合1-Wire总线协议）
    GPIO_InitStruct.Pull = GPIO_NOPULL;          // 输出模式无需内部上拉（依赖外部4.7KΩ电阻）
    //GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW; // 单总线无需高速，低速更稳定
    HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}

// 引脚输入模式配置
void DS18B20_GPIO_Input(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = DS18B20_PIN;       
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;   // 输入模式，释放总线控制权
    GPIO_InitStruct.Pull = GPIO_NOPULL;       // 外部已接4.7KΩ上拉，禁用内部上拉
    //GPIO_InitStruct.Pull = GPIO_PULLUP;  // 启用内部上拉，替代外部4.7KΩ电阻
    HAL_GPIO_Init(DS18B20_PORT, &GPIO_InitStruct);
}

// 复位DS18B20（保持不变）
uint8_t DS18B20_Reset(void)
{
    uint8_t presence = 0;
    
    // 1. 主机拉低总线480us以上
    DS18B20_GPIO_Output();
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    TIM6_DelayUs(500); // 稍大于480us，确保可靠
    
    // 2. 释放总线，等待15-60us后检测存在脉冲
    DS18B20_GPIO_Input();
    TIM6_DelayUs(30); // 在15-60us之间采样
    
    // 3. 检测存在脉冲（DS18B20在此期间会拉低总线）
    if (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == GPIO_PIN_RESET) {
        presence = 1; // 设备存在
    }
    
    // 4. 等待存在脉冲结束（DS18B20拉低60-240us）
    TIM6_DelayUs(240); // 确保存在脉冲结束
    
    return presence; // 1:存在, 0:不存在
}
// 初始化DS18B20（修改时钟使能）
uint8_t DS18B20_Init(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE(); // 使能GPIOD时钟
    return DS18B20_Reset();
}

// 写一个字节到DS18B20（保持不变）
void DS18B20_WriteByte(uint8_t dat)
{
    uint8_t i;
    
    for(i=0; i<8; i++)
    {
        DS18B20_GPIO_Output();
        
        if(dat & 0x01) // 写1
        {
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
            TIM6_DelayUs(6);  // 修正：1-15us
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
            TIM6_DelayUs(60); // 保持总线高电平
        }
        else // 写0
        {
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
            TIM6_DelayUs(60); // 保持60-120us低电平
            HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_SET);
            TIM6_DelayUs(10);
        }
        dat >>= 1;
    }
}

// 从DS18B20读一个字节（优化后）
uint8_t DS18B20_ReadByte(void)
{
    uint8_t i, dat = 0;
    
    for(i=0; i<8; i++)
    {
        dat >>= 1;
        
        DS18B20_GPIO_Output();
        HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
        TIM6_DelayUs(2);  // 至少1us的低电平
        
        DS18B20_GPIO_Input();
        TIM6_DelayUs(10); // 调整采样时间，确保在15us窗口内
        
        if(HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == GPIO_PIN_SET)
            dat |= 0x80;
        
        TIM6_DelayUs(55); // 确保时隙总长度至少60us
    }
    return dat;
}

//uint8_t DS18B20_ReadBit(void)
//{
//    uint8_t bit = 0;
//
//    // 启动读时隙：主机拉低至少 1 us
//    DS18B20_GPIO_Output();
//    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
//    TIM6_DelayUs(2); // >=1us
//    
//    // 释放总线，让从机驱动数据
//    DS18B20_GPIO_Input();
//    TIM6_DelayUs(12); // 在 ~15us 内采样（根据你的 TIM6 精度调整）
//    
//    if (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == GPIO_PIN_SET) {
//        bit = 1;
//    } else {
//        bit = 0;
//    }
//
//    // 补足时隙总长度到 >=60us（防止下一时隙太快）
//    TIM6_DelayUs(50);
//    return bit;
//}
//
//void DS18B20_WriteBit(uint8_t bit)
//{
//    // 启动写时隙：主机拉低
//    DS18B20_GPIO_Output();
//    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
//
//    if (bit) {
//        // 写 '1'：在 1~15us 内释放
//        TIM6_DelayUs(6); // 拉低 ~6us
//        DS18B20_GPIO_Input(); // 释放（注意：若用开漏输出也可以写 HAL_GPIO_WritePin(..., GPIO_PIN_SET)）
//        TIM6_DelayUs(54); // 补足到 >=60us
//    } else {
//        // 写 '0'：保持低电平 >=60us
//        TIM6_DelayUs(60);
//        DS18B20_GPIO_Input(); // 释放
//        TIM6_DelayUs(2); // 恢复时间 >=1us
//    }
//}
// ----------------- 替换：更严格的 ReadBit / WriteBit 实现 -----------------

// 读一个 bit（主机启动读时隙）
// 说明（按数据手册）：
// 1) 主机拉低至少 1us -> 释放 -> 在释放后约 15us 处采样（从机在此处驱动数据）
// 2) 总时隙 >= 60us，槽间恢复时间 >=1us
uint8_t DS18B20_ReadBit(void)
{
    uint8_t bit = 0;

    // 保护时序：短时屏蔽中断，避免被 printf/中断打断
    __disable_irq();

    // 1) 主机拉低至少 1us 启动读时隙
    DS18B20_GPIO_Output();
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);
    TIM6_DelayUs(2); // 拉低 ~2us，满足 >=1us

    // 2) 释放总线，让从机驱动数据
    DS18B20_GPIO_Input();    // 释放总线（外部上拉将拉高）
    // 在释放后的 ~15us 处采样，从机保证在启动后 15us 内输出有效
    TIM6_DelayUs(13);        // 约 13us（加上前面的 2us ~15us），如需严格可改为 14~15

    // 3) 采样数据位
    if (HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == GPIO_PIN_SET) {
        bit = 1;
    } else {
        bit = 0;
    }

    // 4) 补足时隙到 >=60us（已用大约 15us，补足约 45us）
    TIM6_DelayUs(45);

    // 给出至少 1us 恢复时间（这里已包含在上面，额外再短延时以安全）
    // TIM6_DelayUs(1); // 可选

    __enable_irq();
    return bit;
}


// 写一个 bit（遵循时隙：总时隙 >= 60us）
// 说明（按数据手册）：
// 写 1：拉低后在 15us 内释放（通常 6~8us），然后由上拉拉高；总时隙 >=60us。
// 写 0：拉低保持 >=60us，然后释放；总时隙 >=60us。
void DS18B20_WriteBit(uint8_t bit)
{
    __disable_irq();

    // 拉低启动写时隙
    DS18B20_GPIO_Output();
    HAL_GPIO_WritePin(DS18B20_PORT, DS18B20_PIN, GPIO_PIN_RESET);

    if (bit) {
        // 写 '1'：短低脉冲 (<=15us)，然后释放
        TIM6_DelayUs(6);          // 拉低 ~6us，安全小于 15us
        DS18B20_GPIO_Input();     // 释放总线（由上拉拉高）
        // 补足时隙：确保总时隙 >= 60us
        TIM6_DelayUs(54);         // 6 + 54 = 60us
        // 恢复时间已包含
    } else {
        // 写 '0'：保持低电平 >= 60us
        TIM6_DelayUs(60);         // 保持低电平 ~60us
        DS18B20_GPIO_Input();     // 释放总线
        // 恢复时间
        TIM6_DelayUs(2);          // >=1us 恢复（取2us更安全）
    }

    __enable_irq();
}


// 添加CRC校验函数
uint8_t DS18B20_CRC8(uint8_t *data, uint8_t len)
{
    uint8_t crc = 0;
    uint8_t i, j;
    
    for (i = 0; i < len; i++)
    {
        crc ^= data[i];
        for (j = 0; j < 8; j++)
        {
            if (crc & 0x01)
            {
                crc = (crc >> 1) ^ 0x8C;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}


//// 温度读取函数(scratchpad:温度寄存器）
float DS18B20_ReadTemperature(void)
{
    uint8_t temp_h, temp_l, crc, scratchpad[9];
    int16_t temp;
    float temperature;


    // 第一步：复位
    if (!DS18B20_Reset()) {
        return -1000.0f;
    }

    // 发送跳过ROM命令（0xCC）和转换命令（0x44）
    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0x44);

    // 等待转换完成，使用更可靠的等待方式
    DS18B20_GPIO_Input();
    while(HAL_GPIO_ReadPin(DS18B20_PORT, DS18B20_PIN) == GPIO_PIN_RESET);

    // 再次复位，准备读取数据
    if (!DS18B20_Reset()) {
        printf("[DS18B20] Error: Reset failed before reading scratchpad.\r\n");
        return -1000.0f;
    }

    // 发送跳过ROM命令（0xCC）和读暂存器命令（0xBE）
    DS18B20_WriteByte(0xCC);
    DS18B20_WriteByte(0xBE);

    // 读取整个暂存器（9字节）用于CRC校验
    for(int i=0; i<9; i++){
        scratchpad[i] = DS18B20_ReadByte();
    }
    
    temp_l = scratchpad[0];
    temp_h = scratchpad[1];
    crc = scratchpad[8];
    

    // 验证CRC
    if(DS18B20_CRC8(scratchpad, 8) != crc){
        //printf("[DS18B20] Error: CRC check failed!\r\n");
        return -1001.0f;
    }

    // 组合成16位整数
    temp = (temp_h << 8) | temp_l;

    // 计算温度值
    temperature = temp * 0.0625f;

    return temperature;
}

/*
  Match ROM + Read scratchpad -> 返回温度或错误码（与之前单设备读取类似）
  参数：rom[8] 为目标 ROM（LSB first）
*/
float DS18B20_ReadTemperature_ByROM(const uint8_t rom[8])
{
    uint8_t scratch[9];
    int16_t raw;
    float temp_c;

    if (!DS18B20_Reset()) {
        return -1000.0f;
    }

    // Match ROM (0x55) + 8 bytes ROM
    DS18B20_WriteByte(0x55);
    for (uint8_t i = 0; i < 8; i++) DS18B20_WriteByte(rom[i]);

    // 请求转换（或者先全局转换后只读），这里假设转换已完成（主流程会先 Skip ROM：0xCC + 0x44）
    // 复位并读 scratchpad
    if (!DS18B20_Reset()) {
        return -1000.0f;
    }
    DS18B20_WriteByte(0x55);
    for (uint8_t i = 0; i < 8; i++) DS18B20_WriteByte(rom[i]);
    DS18B20_WriteByte(0xBE); // Read Scratchpad

    for (int i = 0; i < 9; i++) scratch[i] = DS18B20_ReadByte();

    // CRC 校验
    if (DS18B20_CRC8(scratch, 8) != scratch[8]) {
        return -1001.0f;
    }

    raw = (int16_t)((scratch[1] << 8) | scratch[0]);
    temp_c = (float)raw * 0.0625f;
    return temp_c;
}

/*
 完整 OneWire Search ROM 实现
*/
uint8_t DS18B20_SearchDevices(uint8_t max_devices)
{
    uint8_t id_bit_number;
    uint8_t last_zero, rom_byte_number, rom_byte_mask;
    uint8_t search_result = 0;
    uint8_t rom_no[8];
    uint8_t last_discrepancy = 0;
    uint8_t last_device_flag = 0;
    uint8_t last_family_discrepancy = 0;

    // 清空之前发现的列表
    ds18_count = 0;
    memset(ds18_roms, 0, sizeof(ds18_roms));

    // 如果总线上没有设备，直接返回 0
    // 注意：Reset 会检测 presence
     if (!DS18B20_Reset()) return 0;


   // 初始化，开始第一次搜索
    last_discrepancy = 0;
    last_device_flag = 0;
    last_family_discrepancy = 0;

    while (!last_device_flag && (search_result < max_devices)) {
        // reset per-search state
        id_bit_number = 1;
        last_zero = 0;
        rom_byte_number = 0;
        rom_byte_mask = 1;
        memset(rom_no, 0, 8);

        // 复位并检测 presence（每轮搜索前推荐复位）
        if (!DS18B20_Reset()) {
            // 总线上无设备
            break;
        }

        // 发送 Search ROM 命令 0xF0
        DS18B20_WriteByte(0xF0);
        // 遍历 64 位 ROM
        do {
            uint8_t id_bit = DS18B20_ReadBit();      // 读取该位
            uint8_t cmp_id_bit = DS18B20_ReadBit(); // 读取该位的反位（互补位）

            if ((id_bit == 1) && (cmp_id_bit == 1)) {
                // 没有设备回应该搜索（线浮空或无设备）
                // 结束当前搜索循环（失败）
                id_bit_number = 0; // 标记失败
                break;
            } else {
                uint8_t search_direction; // 要写入总线的选择位 (0 or 1)

                if (id_bit != cmp_id_bit) {
                    // 无冲突，选择实际存在的位
                    search_direction = id_bit; // 直接选择哪一位
                } else {
                      // 冲突：两个设备在该位不同
                      if (id_bit_number < last_discrepancy) {
                            // 以前在此位的选择：参考上一次找到的 ROM（index = search_result - 1）
                            // 注意：只有当已有已找到设备 (search_result > 0) 时才能这样参考
                            uint8_t prevRomBit = 0;
                            if (search_result > 0) {
                                prevRomBit = (ds18_roms[search_result - 1][rom_byte_number] & rom_byte_mask) ? 1 : 0;
                            } else {
                                prevRomBit = 0; // 保底
                            }
                            search_direction = prevRomBit;
                      } else if (id_bit_number == last_discrepancy) {
                        // 按规范：在 last_discrepancy 位选择 1
                           search_direction = 1;
                      } else {
                        // 否则选择 0（优先走 0 的分支）
                           search_direction = 0;
                      }

                    // 当选择了 0 的分支，记录为 last_zero 的候选
                    if (search_direction == 0) {
                        last_zero = id_bit_number;
                        if (id_bit_number < 9) {
                            last_family_discrepancy = id_bit_number;
                        }
                    }
                }

                // 将选择位写回总线
                DS18B20_WriteBit(search_direction);

                // 将该选择位写入 rom_no 缓存（LSB first）
                if (search_direction == 1) {
                    rom_no[rom_byte_number] |= rom_byte_mask;
                }

                // 前进到下一位
                id_bit_number++;
                rom_byte_mask <<= 1;
                if (rom_byte_mask == 0) {
                    rom_byte_number++;            
                    rom_byte_mask = 1;
                }
            }
        } while (rom_byte_number < 8);

        // 检查是否成功读取完整 64 位 ROM
        if (id_bit_number == 0) {
            // 搜索失败（没有设备）
            break;
        }

        // 保存找到的 ROM 到列表中
        for (uint8_t i = 0; i < 8; i++) ds18_roms[search_result][i] = rom_no[i];
        search_result++;

        // 设置下一轮搜索的 last_discrepancy
        last_discrepancy = last_zero;

        if (last_discrepancy == 0) {
            last_device_flag = 1; // 没有更多分叉，已找到最后一个设备
        }

        // 注意：按照规范应该持续搜索直到 last_device_flag 为 true（这里循环会自然继续）
        // 但为了防止死循环，也限制搜索次数为 max_devices（上层由 while 控制）
    }

    ds18_count = search_result;
    return ds18_count;
}
// === END ADDED ===

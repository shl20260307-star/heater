/**
  ******************************************************************************
  * 文件名程: main.c 
  * 作    者: 硬石嵌入式开发团队
  * 版    本: V1.0
  * 编写日期: 2017-03-30
  * 功    能: 使用串口通信实现发送数据到电脑以及接收电脑端发送的数据
  ******************************************************************************
  * 说明：
  * 本例程配套硬石stm32开发板YS-F4Pro使用。
  * 
  * 淘宝：
  * 论坛：http://www.ing10bbs.com
  * 版权归硬石嵌入式开发团队所有，请勿商用。
  ******************************************************************************
  */
/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include "usart/bsp_usartx.h"
#include "string.h"
#include "stdio.h"
#include "bsp_steppingmotor.h"
#include "bsp_Limitsensor.h"
#include "bsp_ad.h"
#include <math.h>     // sinf, sqrtf
#include "adc_dma.h" // 在 main.c 全局区定义 DMA 缓冲区（与 adc_dma.h 中 extern 对应）
#include "bsp_dac.h"
#include "ds18b20_multi.h"
#include "heatcool_ctrl.h"
#include "gpio_pins.h"
#include "solution_intensity.h"




#define RX_BUFFER_SIZE 256  // 定义缓冲区大小
unsigned  char data[10];
uint16_t rxIndex = 0,m = 0,n=0;  // 接收缓冲区的索引
//volatile uint32_t interruptCount = 0,j=0;

uint8_t step=0;//状态变量初始化为0 在函数中必须为静态变量,用作状态机的状态指示器,确保数据按照预定的格式和顺序被正确处理
uint8_t frameBuf[RX_BUFFER_SIZE],index=0,length=0,*pFunction;//length记录帧长信息
uint8_t aRxBuffer1[RX_BUFFER_SIZE];
uint8_t sampleData[RX_BUFFER_SIZE];
uint16_t crc16;
//userEnd
uint8_t aRxBuffer,indexFunction;// 定义接收缓冲区,uint8_t通常用于表示一个字节的数据
uint8_t reBuffer[RX_BUFFER_SIZE];

// 在全局变量区定义
uint8_t need_homing = 0; // 0:不需要归位，1:需要归位
int holes[96][2] = {0}; // 用于存储选中孔的坐标
TIM_HandleTypeDef htim6;
TIM_HandleTypeDef htim7;

extern UART_HandleTypeDef husartx;

//ADC模块定义
// 用于保存转换计算后的电压值	 
float ADC_ConvertedValueLocal;
// AD转换结果值
__IO uint16_t ADC_ConvertedValue;

// ================= 命令缓冲（中断 → 主循环） =================
volatile uint8_t  cmd_ready = 0;      // 命令就绪标志
volatile uint8_t  cmd_code = 0;       // 功能码
uint8_t           cmd_buf[RX_BUFFER_SIZE]; // 命令完整帧
uint16_t          cmd_len = 0;        // 帧长度
static uint8_t    ctrl_report_enable = 0;
// =============================================================


/**
  * 函数功能: 系统时钟配置
  * 输入参数: 无
  * 返 回 值: 无
  * 说    明: 无
  */
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
 
  __HAL_RCC_PWR_CLK_ENABLE();                                     //使能PWR时钟

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);  //设置调压器输出电压级别1

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;      // 外部晶振，8MHz
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;                        //打开HSE (外部时钟源）
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;                    //打开PLL
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;            //PLL时钟源选择HSE
  RCC_OscInitStruct.PLL.PLLM = 8;                                 //8分频MHz
  RCC_OscInitStruct.PLL.PLLN = 336;                               //336倍频
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;                     //2分频，得到168MHz主时钟,RCC_PLLP_DIV2=2
  RCC_OscInitStruct.PLL.PLLQ = 7;                                 //USB/SDIO/随机数产生器等的主PLL分频系数
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;       // 系统时钟：168MHz
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;              // AHB时钟： 168MHz.总线连内存、DMA 等，跑满速 168 MHz。
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;               // APB1时钟：42MHz，连 UART、TIM2~7、I2C 等，限制在 42 MHz（低速外设）
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;               // APB2时钟：84MHz，连 ADC、TIM1/8、SPI1 等，限制在 84 MHz。
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  HAL_RCC_EnableCSS();                                            // 使能CSS功能，优先使用外部晶振，内部时钟源为备用
  
 	// HAL_RCC_GetHCLKFreq()/1000    1ms中断一次
	// HAL_RCC_GetHCLKFreq()/100000	 10us中断一次
	// HAL_RCC_GetHCLKFreq()/1000000 1us中断一次
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);                // 配置并启动系统滴答定时器，常用于 HAL 库的延时函数（HAL_Delay() 就靠它）。
  /* 系统滴答定时器时钟源 */
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* 系统滴答定时器中断优先级配置 运维，后端太卷，*/
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

//printf函数重定向,当执行到printf函数时，该函数会被内部调用，会被发送到串口调试助手上显示
int fputc(int ch, FILE *f)
{
	HAL_UART_Transmit(&husartx, (uint8_t *)&ch, 1, 0xFFFF);//huart3为重定向的串口
	return ch;
}

void TIM6_Config(void) {
    // 使能 TIM6 时钟
    __HAL_RCC_TIM6_CLK_ENABLE();

    // 配置 TIM6 定时器
    htim6.Instance = TIM6;  // 选择 TIM6 定时器
//    htim6.Init.Prescaler = (SystemCoreClock / 1000000) - 1;  // 预分频器（2us 计数），将时钟频率分频为 0.5MHz
//   htim6.Init.Period = 0xFFFF;  // 自动重装载值，设置为最大值 0xFFFF（16 位定时器）
    htim6.Init.Prescaler=83;
    htim6.Init.Period = 65535;
    
    htim6.Init.CounterMode = TIM_COUNTERMODE_UP;  // 计数器模式为向上计数
    htim6.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;  // 时钟分频为 1（不分频）
    HAL_TIM_Base_Init(&htim6);  // 初始化 TIM6 定时器
    HAL_TIM_Base_Start(&htim6);  // 启动 TIM6 定时器
}

void TIM6_DelayUs(uint16_t us) {
    // 清零 TIM6 计数器的当前值
    __HAL_TIM_SET_COUNTER(&htim6, 0);

    // 等待计数器值达到指定的微秒数（每个计数对应 2μs）
    while (__HAL_TIM_GET_COUNTER(&htim6) < us);
}
void TIM7_Config(void) {
    // 使能 TIM7 时钟
    __HAL_RCC_TIM7_CLK_ENABLE();

    // 配置 TIM7 定时器
    htim7.Instance = TIM7;  // 选择 TIM7 定时器
    htim7.Init.Prescaler = (SystemCoreClock / 100000) - 1;  // 预分频器，将时钟频率分频为 0.05MHz
    htim7.Init.Period = 0xFFFF;  // 自动重装载值，设置为最大值 0xFFFF（16 位定时器）
    htim7.Init.CounterMode = TIM_COUNTERMODE_UP;  // 计数器模式为向上计数
    htim7.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;  // 时钟分频为 1（不分频）
    HAL_TIM_Base_Init(&htim7);  // 初始化 TIM7 定时器
    HAL_TIM_Base_Start(&htim7);  // 启动 TIM7 定时器
}
void TIM7_DelayUs(uint16_t us) {
    // 清零 TIM7 计数器的当前值
    __HAL_TIM_SET_COUNTER(&htim7, 0);

    // 等待计数器值达到指定的微秒数（每个计数对应 20 μs）
    while (__HAL_TIM_GET_COUNTER(&htim7) < us);
}

static float ReadDs18Average(uint8_t *valid_count_out)
{
    static const uint16_t ds18_pins[4] = {
        GPIO_PIN_11,
        GPIO_PIN_10,
        GPIO_PIN_9,
        GPIO_PIN_8
    };
    float sum = 0.0f;
    uint8_t valid_count = 0;
    uint8_t started_count = 0;
    uint8_t i;

    for (i = 0; i < 4; i++) {
        if (DS18B20_StartConversion_PortPin(GPIOD, ds18_pins[i])) {
            started_count++;
        }
    }

    if (started_count == 0) {
        if (valid_count_out) {
            *valid_count_out = 0;
        }
        return -127.0f;
    }

    HAL_Delay(750);

    for (i = 0; i < 4; i++) {
        float t = DS18B20_ReadTemperature_PortPin(GPIOD, ds18_pins[i]);
        if (t > -100.0f && t < 200.0f) {
            sum += t;
            valid_count++;
        }
    }

    if (valid_count_out) {
        *valid_count_out = valid_count;
    }

    if (valid_count == 0) {
        return -126.0f;
    }

    return sum / valid_count;
}

  static const char* HeatCoolModeToText(HeatCoolMode_t mode)
  {
    if (mode == HEATCOOL_MODE_HEAT) {
      return "HEAT";
    }
    if (mode == HEATCOOL_MODE_COOL) {
      return "COOL";
    }
    return "OFF";
  }


// CRC16 校验函数
static uint16_t CRC16_Check(const uint8_t *data,uint8_t len)
{
    uint16_t CRC16 = 0xFFFF;
    uint8_t state,i,j;
    for(i = 0; i < len; i++ )
    {
        CRC16 ^= data[i];
        //printf("Byte[%d]: 0x%02X, CRC: 0x%04X\n", i, data[i], CRC16);
        for( j = 0; j < 8; j++)
        {
            state = CRC16 & 0x01;
            CRC16 >>= 1;
            if(state)
            {
                CRC16 ^= 0xA001;
            }
        }
    }
    //printf("CRC: 0x%04X\n",CRC16);

    return CRC16;
}

void dataPrint(uint8_t* datas,uint16_t size){
    printf("syz\r\n");

   for (uint16_t i = 0; i < size; i++) {
        printf("%02X", datas[i]); // 以十六进制格式打印每个字节
    }
    printf("\r\n"); // 打印换行
}

// 采样数据帧格式：
// [帧头(0x3A)][帧长][功能码][数据区][CRC(2字节)][帧尾(0x0D, 0x0A)]
// 数据区格式： [孔数量(1字节)][孔1行(1字节)][孔1列(1字节)]...[孔N行][孔N列]
//解析采样孔位置信息帧数据
void ParseFrameAndMove(uint8_t *frameData) {
    //printf("Columnaa\n");
    // 孔板的8行12列
    
    uint8_t num_holes = frameData[0];// 数据区从第四个字节开始，首先提取孔数量，该数据位于索引 3
    //printf("Parsenum_holes: %d\n",num_holes);

    
      int hole_count =0;//选中孔的数量
      int data_index = 1;// 数据区解析起始位置为索引4

    
      // 循环遍历数据区，解析每个孔的数据（每个孔占2字节）
      for (int i = 0; i < num_holes; i++) {
          holes[hole_count][0] = frameData[data_index]; // 解析当前孔的行号（存储在数据区当前索引处），放入全局变量 holes 数组的对应位置
          holes[hole_count][1] = frameData[data_index + 1];  // 解析当前孔的列号，放入 holes 数组对应位置的第二个字节      
          data_index += 2;// 数据区索引增加 2（跳过已解析的2个字节）
          hole_count++;// 孔计数器加1
          // 打印当前扫描的孔信息
          //printf("R: %02X, C: %02X\n", holes[i][0], holes[i][1]);
      }
      scanAllHoles(holes,hole_count);
    
    
}
// 状态机复位
void ResetStateMachine(void)
{
    step = 0;              // 状态机状态复位
    index = 0;             // 数组索引复位
    length = 0;            // 帧长信息复位
    pFunction = NULL;      // 指针复位
    memset(frameBuf, 0, RX_BUFFER_SIZE);  // 清空接收缓冲区数据
}

//帧数据处理回传
void dataProcess(uint8_t* datas, uint16_t size, uint8_t functionCode){

//void dataProcess(uint8_t* datas,uint16_t size,uint8_t* pFunction){
   // 1. 读取功能码
   //uint8_t functionCode = *pFunction;
   // 2.根据功能码处理数据
    switch (functionCode) {
        case 0xF0: // 开机
          {
              //printf("Received: %02X\n", functionCode); // 打印接收到的数据

              //dataPrint(datas,size);
              xyReset();//电机归位
          }
            break;
        case 0x31://扫描样品(优化路径）
          {
              uint8_t j=0;
              memset(sampleData, 0, sizeof sampleData);
              for (uint16_t i = 3; i < size; i++) {
                sampleData[j]=datas[i];
                j++;
                //printf("%02X", datas[i]); // 以十六进制格式打印每个字节
              }

              uint8_t num_holes = sampleData[0];
              
              if (num_holes > 14) {
                    uint8_t *bmp = &sampleData[1];       // 后面紧跟 12 字节位图
                    int hole_count = 0;
                    for (int byte = 0; byte < 12; ++byte) {
                      for (int bit = 0; bit < 8; ++bit) {
                        if (bmp[byte] & (1 << bit) && hole_count < num_holes) {
                          holes[hole_count][0] = (byte*8 + bit) / 12;   // row
                          holes[hole_count][1] = (byte*8 + bit) % 12;   // col
                          // 打印当前扫描的孔信息
                          //printf("R: %02X, C: %02X\n", holes[hole_count][0], holes[hole_count][1]);
                          hole_count++;
                        }
                        

                      }
                    }
                
                   scanAllHoles(holes, num_holes);
                   
              }else{//多个孔扫描
                   ParseFrameAndMove(sampleData);//解析孔位置信息
             }
          }
          break;

       case 0x06://光源波段选择
         { //printf("0x06\n");
              if(datas[3]==0x44){//紫光顺时针转60度
                  //printf("0x44\n");
                  ForwardRotate(60);
                  dataPrint(datas,size);
              }else{             //红光顺时针转120度
                  //printf("0x66\n");
                  ForwardRotate(120);
              }
         }
          break;
       case 0xC8://转轮归位
               RotateFilterWheel();
      
 
          break;        
       case 0x37:     
              if(datas[3]==0xF0){//震荡样品 
//                    printf("0xF0\n");
                    shakePlate();
                          
              }else if(datas[3]==0x00){  //弹出样品 
                  
                  PopPlate();
                    
              }else{
                  LoadPlate();
                  //printf("加载\n");
              }
          break;         
        case 0xB7: // 读取四路 DS18B20 温度并回传平均值，保持 Qt 串口格式不变
              {
            uint8_t valid_count = 0;
            float avg = ReadDs18Average(&valid_count);

            if (avg <= -127.0f) {
              const char *err = "TEMP:ERR_RESET\r\n";
              HAL_UART_Transmit(&husartx, (uint8_t*)err, strlen(err), 100);
              break;
            }

                char txBuf[64];
                if (valid_count > 0) {
                    int len = snprintf(txBuf, sizeof(txBuf), "TEMPAVG:%.4f\r\n", avg);
                    HAL_UART_Transmit(&husartx, (uint8_t*)txBuf, len, 100);
                } else {
                    const char *err = "TEMP:ERR_ALL_FAIL\r\n";
                    HAL_UART_Transmit(&husartx, (uint8_t*)err, strlen(err), 100);
                }
              }
          break;
       case 0xB8: // 设置目标温度（单位0.01°C）并开启温控
              {
                int16_t target_centi = (int16_t)(((uint16_t)datas[3] << 8) | datas[4]);
                float target = ((float)target_centi) / 100.0f;
                char txBuf[64];

                if (target < -20.0f || target > 120.0f) {
                    const char *err = "SET_TARGET:ERR_RANGE\r\n";
                    HAL_UART_Transmit(&husartx, (uint8_t*)err, strlen(err), 100);
                    break;
                }

                HeatCool_SetTargetC(target);
                HeatCool_Enable(1);
                ctrl_report_enable = 1;

                {
                    int len = snprintf(txBuf, sizeof(txBuf), "SET_TARGET:%.2f\r\n", target);
                    HAL_UART_Transmit(&husartx, (uint8_t*)txBuf, len, 100);
                }
              }
          break;
       case 0xBA: // 停止温控并停止周期上报
              {
                const char *msg = "CTRL:STOP\r\n";
                HeatCool_Enable(0);
                ctrl_report_enable = 0;
                HAL_UART_Transmit(&husartx, (uint8_t*)msg, strlen(msg), 100);
              }
          break;
       case 0xA1:
           //printf("iar0xA1\n");

            uint8_t mask = datas[3]; // 数据区第一个字节为掩码
           

            if (mask & 0x01) // bit0 -> PE0
            { Toggle_PE0(); 
//              uint8_t pe0_state = Toggle_PE0_and_GetState();//保证在点击翻转按钮之后测得当前引脚的值
//              char message[20];
//              snprintf(message, sizeof(message), "PE0:%d\n", pe0_state);
//              HAL_UART_Transmit(&husartx, (uint8_t*)message, strlen(message), 100);  // 发送电平状态信息

            }  
            if (mask & 0x02) // bit1 -> PI6
            { 
              
              Toggle_PI6(); 
//              uint8_t pi6_state = Toggle_PI6_and_GetState();
//              char message[20];
//              snprintf(message, sizeof(message), "PI6:%d\n",pi6_state);
//              HAL_UART_Transmit(&husartx, (uint8_t*)message, strlen(message), 100);  // 发送电平状态信息

            }  
            
            
            break;
            
       case 0x0C: // 设置 光强DAC 值（不回传任何帧/不打印）
            // 约定：数据区第一、第二字节为 DAC 高、低字节（uint16），范围 0~4095
            // datas[3] 作为数据区即命令内容首字节，因此这里也沿用该约定:
            // datas[3] = DATA_H, datas[4] = DATA_L
            //if (size >= 6) { // 最少应包含 [HDR][LEN][FUNC][DH][DL][CRC..][TAIL..]，做个简单边界检查
            uint16_t dh = (uint8_t)datas[3];
            uint16_t dl = (uint8_t)datas[4];
            uint16_t dac2_val = (dh << 8) | dl;
            HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac2_val);

            printf("dac2：%d\n", dac2_val);

            if (dac2_val > 4095){
                  dac2_val = 4095; // 截断到12bit

                  HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, dac2_val);
                  printf("dac2：", dac2_val);

            }else {
                // 如果你愿意记录错误，也不要用 printf（会回串口），可考虑设置错误计数变量或点亮 LED
                // error_count++;
            }
        
        break;
//        case 0xD4:
//        {
//         
//        // 先跑一轮但不算(必要性代码，否则测得的数据不稳定忽高忽低）
//        ADC_DMA_Start(adc_dma_buffer, SAMPLES_COUNT);
//        HAL_Delay(500);   // 等 DMA 跑完
//        HAL_ADC_Stop_DMA(&hadcx);
//        __HAL_ADC_DISABLE(&hadcx);
// hadcx.Instance->CR2 &= ~ADC_CR2_DMA; // 清除 DMA 使能位
//
//        
//        SolutionIntensity_StartSample();   // 采样，且只采样只测试一次
//        HAL_Delay(500);  // 停顿 0.5秒

          
//        }
//          break;

       default:
                printf("Columna2\n");
                dataPrint(datas,size);
          break;
    }

  

}



//处理上位机发过来的命令数据帧
void ProcessReceiveData(uint8_t aRxBuffer){
  
  if (index >= RX_BUFFER_SIZE)
  {
      ResetStateMachine();
      return;
  }

//   static uint8_t step=0;//状态变量初始化为0 在函数中必须为静态变量,用作状态机的状态指示器,确保数据按照预定的格式和顺序被正确处理
//   static uint8_t frameBuf[RX_BUFFER_SIZE],index=0,length=0,*pFunction;//length记录帧长信息
   //printf("rece: %02X\n", aRxBuffer); // 打印接收到的数据
   switch(step)
   {
      case 0://接收帧头1状态
          if(aRxBuffer== 0x3A || aRxBuffer== 0x3a){
            //printf("head\r\n");
            step++;
            index=0;                                                  
            frameBuf[index++]=aRxBuffer;
          }
          break;
      case 1://帧长信息
          step++;
          frameBuf[index++]=aRxBuffer;
          length=aRxBuffer;    
          break;
      case 2://功能码信息
          frameBuf[index++]=aRxBuffer;
          pFunction=&frameBuf[index-1];//标记功能码所在地址
          if (length - index == 4) {
            // 无数据区帧：功能码后直接进入 CRC 处理
            step = 4;
          } else {
            step = 3;
          }
          
          break;
      case 3: //帧命令内容 
        frameBuf[index++]=aRxBuffer;
        if(length-index == 4){//index已经指到了CRC校验位,用于处理内容较长的命令
          step++;
        }
        break;
      case 4:////接收crc16校验高8位字节
        step++;
        crc16 = aRxBuffer;
        break;
      case 5://接收crc16校验低8位字节
        crc16 <<= 8;
        crc16 += aRxBuffer;
        
        if(crc16 == CRC16_Check(frameBuf,index))//校验正确进入下一状态
        {
            step ++;
        }
        else
        {
            ResetStateMachine();//状态机复位
            printf("ERROR_CCRC16\r\n");

        }
        break;
      case 6://帧尾判断
        if(aRxBuffer== 0x0D || aRxBuffer== 0x0d){

            step++;
            frameBuf[index++]=aRxBuffer;

        }else{
            ResetStateMachine();//状态机复位
            printf("ERROR_case6\r\n");

        }
        break;
      case 7:
        
        if (aRxBuffer == 0x0A || aRxBuffer == 0x0a)
        {
            frameBuf[index++] = aRxBuffer;

            // ======= 关键修改：只缓存命令，不执行 =======
            memcpy(cmd_buf, frameBuf, index); // 拷贝完整帧
            cmd_len  = index;
            cmd_code = *pFunction;            // 保存功能码
            cmd_ready = 1;                    // 通知主循环
            // ============================================

            ResetStateMachine();              // 状态机复位
        }
        else
        {
            ResetStateMachine();
        }
        break;

//        if(aRxBuffer== 0x0A || aRxBuffer== 0x0a){//帧尾判断
//          
//          
//            frameBuf[index++]=aRxBuffer;
//            
//            dataProcess(frameBuf,index,pFunction);//功能码判断，执行相应操作
//            ResetStateMachine();//状态机复位
//           
//        }
//        else if(aRxBuffer == 0x3A || aRxBuffer== 0x3a){
//          printf("case77false: %02X\n", aRxBuffer); // 打印接收到的数据
//          ResetStateMachine();//状态机复位
//        }else{
//           ResetStateMachine();//状态机复位
//           printf("step7777false: %d\n", step); // 打印接收到的数据
//
//        }
//        break;
        
      default:printf("error");break;//多余状态，正常情况下不可能出现     
        
   }

}
void ExecuteCommand(void)
{
    if (!cmd_ready) return;

    cmd_ready = 0;   // 先清标志，防止重入

    // 这里，才是真正执行“耗时操作”
    dataProcess(cmd_buf, cmd_len, cmd_code);
}
int main(void)
{
  
  //printf("syz/n");
  /* 复位所有外设，初始化Flash接口和系统滴答定时器 */
  HAL_Init();
  /* 配置系统时钟 */
  SystemClock_Config();
  
  /* 初始化串口并配置串口中断优先级 */
  MX_USARTx_Init();
  TIM6_Config();
  Limitsensor_Init();
  steppingmotor_Init();
  MX_GPIO_PE_PI_Init();
  
  Toggle_PE0();
 
  TIM2_Init();       // 新增：配置 TIM2 硬件触发，1 kHz TRGO
  /*如果 DMA_Init() 在 MX_ADCx_Init() 之后调用，会导致 DMA 时钟和流配置无效，使 hdma_adc 句柄未真正初始化，从而回调永远不会触发?*/
  DMA_Init();        // 配置好正确的 DMA 流 & 通道 
  MX_ADCx_Init();        // 已经开启硬触发 & DMA 请求 
  ADC_DMA_Start(adc_dma_buffer, SAMPLES_COUNT);
  
  
 
   /* 初始化DAC */
  MX_DAC_Init();
  /* 输出指定电压 */
  uint32_t dac1_val = (uint32_t)(4095 * 1.0/ 2.5); // 输出给光电传感器,增益
  //uint32_t dac2_val = (uint32_t)(4095 * 1.05/ 2.5); // 输出控制电阻大小
  HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, dac1_val);// 输出1802-1.1v, 1310-0.8V (约为 3.3V * 2048 / 4095)
  //HAL_DAC_SetValue(&hdac, DAC_CHANNEL_2, DAC_ALIGN_12B_R, 1750);

  /* 启动DAC */
  HAL_DAC_Start(&hdac, DAC_CHANNEL_1);
  HAL_DAC_Start(&hdac, DACx_CHANNEL2);

  HeatCool_Init();

  
  DS18B20_GPIOs_Init();
  
  /* 使能接收，进入中断回调函数 ,当接收到数据时，将调用回调函数*/
  HAL_UART_Receive_IT(&husartx,aRxBuffer1,1);//每次中断只接受一个字节的数据

  //printf("Received: %d\n", aRxBuffer); // 打印接收到的数据

  HAL_GPIO_WritePin(X_MF_PORT, X_MF_PIN, GPIO_PIN_SET);// 初始管脚电平置1，锁定电机
  HAL_GPIO_WritePin(Y_MF_PORT, Y_MF_PIN, GPIO_PIN_SET);// 初始管脚电平置1，锁定电机
  

  /* 无限循环 */
  while (1)
  {
    static uint32_t last_ctrl_ms = 0;
    static float last_avg_temp = -127.0f;
    //printf("0x55/n");
    ExecuteCommand();   // ★★★ 新增：执行串口命令

    if (HeatCool_IsEnabled()) {
      uint32_t now = HAL_GetTick();
      if ((now - last_ctrl_ms) >= 500U) {
        uint8_t valid_count = 0;
        float avg = ReadDs18Average(&valid_count);
        if (valid_count > 0 && avg > -100.0f && avg < 200.0f) {
          last_avg_temp = avg;
          HeatCool_ControlStep(avg);
        } else {
          last_avg_temp = -127.0f;
        }

        if (ctrl_report_enable) {
          char ctrlBuf[96];
          int len = snprintf(ctrlBuf,
                             sizeof(ctrlBuf),
                             "CTRL:CUR=%.2f,TGT=%.2f,MODE=%s,PWR=%u\r\n",
                             last_avg_temp,
                             HeatCool_GetTargetC(),
                             HeatCoolModeToText(HeatCool_GetMode()),
                             HeatCool_GetPowerRaw());
          HAL_UART_Transmit(&husartx, (uint8_t*)ctrlBuf, len, 100);
        }
        last_ctrl_ms = now;
      }
    }

    HAL_Delay(50);
    
    /* 3.3为AD转换的参考电压值，stm32的AD转换为12bit，2^12=4096，即当输入为3.3V时，AD转换结果为4096 */
 //   ADC_ConvertedValueLocal =(double)ADC_ConvertedValue*3.3/4096; 	
//    printf("AD转换原始值 = 0x%04X \r\n", ADC_ConvertedValue); 
//    printf("计算得出电压值 = %f V \r\n",ADC_ConvertedValueLocal); 
  }
  
 
  
}




//userEnd
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{
    //printf("syz1\r\n");

    //printf("Recei: %02X\n", aRxBuffer1[0]); // 打印接收到的数据

   ProcessReceiveData(aRxBuffer1[0]);//处理接收到的数据
      
   // printf("syz2\r\n");
   //HAL_UART_Receive_IT(&husartx,&aRxBuffer,1);//带着IT是有中断的
   // HAL_UART_Receive_IT(&husartx,reBuffer,1);//带着IT是有中断的
      
   HAL_UART_Receive_IT(&husartx,aRxBuffer1,1);//带着IT是有中断的

} 


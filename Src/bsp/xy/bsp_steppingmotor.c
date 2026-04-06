#include "bsp_steppingmotor.h"
#include "math.h"
#include <stdlib.h>
#include "bsp_Limitsensor.h"
#include <stdbool.h>
#include "bsp_dac.h"
#include "solution_intensity.h"
#include "bsp_ad.h"
#include "adc_dma.h"



extern void TIM6_DelayUs(uint16_t us);//微秒级


#define PULSES_PER_CM ((double)(100000.0/2.5)) // 计算每厘米脉冲数，强制转换为 double,十万个脉冲移动2.6cm,每移动一厘米需要多少脉冲
#define HOLE_SPACING 0.9              // 孔间距 0.9cm
#define AVERAGE_ANGLE ((double)(1.8/200))//每个脉冲对应的角度

#define HOME_OFFSET_X  -9.3//电机归位到孔A1的距离坐标
#define HOME_OFFSET_Y  11.2

static bool firstScan = true;
static int last_x = 0, last_y = 0;  // 上次扫描结束时的列、行


/*.Pin：指定要操作的引脚编号（如 GPIO_PIN_6 表示PC6）。
.Pull：设置内部上拉或下拉电阻：
GPIO_PULLUP：引脚默认高电平（悬空时读为1）。
GPIO_PULLDOWN：引脚默认低电平（悬空时读为0）。
限位传感器引脚启用上拉，未触发时读高电平，触发时接地读低电平。*/

/**
* 函数功能: steppingmotor初始化函数
*/
void steppingmotor_Init()
{
    GPIO_InitTypeDef GPIO_InitStruct = {0}; // 定义结构体变量

    // 使能端口时钟
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOE_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();

    // 公共配置
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;       // 设置为推挽输出模式
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;     // 速度为100MHz
    GPIO_InitStruct.Pull = GPIO_PULLUP;               // 上拉

    // 初始化X轴PU_PIN
    GPIO_InitStruct.Pin = X_PU_PIN;                   // 管脚设置
    HAL_GPIO_Init(X_PU_PORT, &GPIO_InitStruct);       // 初始化结构体
    HAL_GPIO_WritePin(X_PU_PORT, X_PU_PIN, GPIO_PIN_SET); // 初始管脚电平置1

    // 初始化X轴DR_PIN
    GPIO_InitStruct.Pin = X_DR_PIN;                   // 管脚设置
    HAL_GPIO_Init(X_DR_PORT, &GPIO_InitStruct);       // 初始化结构体
    HAL_GPIO_WritePin(X_DR_PORT, X_DR_PIN, GPIO_PIN_SET); // 初始管脚电平置1，正向顺时针旋转

    // 初始化X轴MF_PIN
    GPIO_InitStruct.Pin = X_MF_PIN;                   // 管脚设置
    HAL_GPIO_Init(X_MF_PORT, &GPIO_InitStruct);       // 初始化结构体
    HAL_GPIO_WritePin(X_MF_PORT, X_MF_PIN, GPIO_PIN_SET); // 初始管脚电平置1，锁住电机

    // 初始化Y轴PU_PIN
    GPIO_InitStruct.Pin = Y_PU_PIN;                   // 管脚设置
    HAL_GPIO_Init(Y_PU_PORT, &GPIO_InitStruct);       // 初始化结构体
    HAL_GPIO_WritePin(Y_PU_PORT, Y_PU_PIN, GPIO_PIN_SET); // 初始管脚电平置1
    
    // 初始化Y轴DR_PIN
    GPIO_InitStruct.Pin = Y_DR_PIN;                   // 管脚设置
    HAL_GPIO_Init(Y_DR_PORT, &GPIO_InitStruct);       // 初始化结构体
    HAL_GPIO_WritePin(Y_DR_PORT, Y_DR_PIN, GPIO_PIN_SET); // 初始管脚电平置1，正向顺时针旋转

    // 初始化Y轴MF_PIN
    GPIO_InitStruct.Pin = Y_MF_PIN;                   // 管脚设置
    HAL_GPIO_Init(Y_MF_PORT, &GPIO_InitStruct);       // 初始化结构体
    HAL_GPIO_WritePin(Y_MF_PORT, Y_MF_PIN, GPIO_PIN_SET); // 初始管脚电平置1，锁住电机
    
    // 初始化z轴PU_PIN
    GPIO_InitStruct.Pin = z_PU_PIN;                   // 管脚设置
    HAL_GPIO_Init(z_PU_PORT, &GPIO_InitStruct);       // 初始化结构体
    HAL_GPIO_WritePin(z_PU_PORT, z_PU_PIN, GPIO_PIN_SET); // 初始管脚电平置1

    // 初始化z轴DR_PIN
    GPIO_InitStruct.Pin = z_DR_PIN;                   // 管脚设置
    HAL_GPIO_Init(z_DR_PORT, &GPIO_InitStruct);       // 初始化结构体
    HAL_GPIO_WritePin(z_DR_PORT, z_DR_PIN, GPIO_PIN_SET); // 初始管脚电平置1，正向顺时针旋转

    // 初始化z轴MF_PIN
    GPIO_InitStruct.Pin = z_MF_PIN;                   // 管脚设置
    HAL_GPIO_Init(z_MF_PORT, &GPIO_InitStruct);       // 初始化结构体
    HAL_GPIO_WritePin(z_MF_PORT, z_MF_PIN, GPIO_PIN_SET); // 初始管脚电平置1，锁住电机
    
}

/**
 * @brief  驱动滤光片转轮反转 ，逆时针
 */
void ReverseRotate(int angle) {

        // 设置旋转方向（根据实际方向调整）
      HAL_GPIO_WritePin(z_DR_PORT, z_DR_PIN, GPIO_PIN_RESET);
      unsigned long pulsesNum = (unsigned long)(angle/AVERAGE_ANGLE + 0.5); 
  
      
      for(int i=0;i<pulsesNum;i++){
        // 生成一个步进脉冲
        HAL_GPIO_WritePin(z_PU_PORT, z_PU_PIN, GPIO_PIN_RESET); // 低电平
        TIM6_DelayUs(60);  // 脉冲间隔
        HAL_GPIO_WritePin(z_PU_PORT, z_PU_PIN, GPIO_PIN_SET);   // 高电平
        TIM6_DelayUs(80); // 脉冲间隔
      }
      //printf("%d\n",i);

    
  
}

/**
 * @brief  驱动滤光片转轮正转，顺时针
 */
void ForwardRotate(int angle) {
      unsigned long pulsesNum = (unsigned long)(angle/AVERAGE_ANGLE + 0.5); 
      // 设置旋转方向（根据实际方向调整）
      HAL_GPIO_WritePin(z_DR_PORT, z_DR_PIN, GPIO_PIN_SET);
      for(int i=0;i<pulsesNum;i++){
        // 生成一个步进脉冲
        HAL_GPIO_WritePin(z_PU_PORT, z_PU_PIN, GPIO_PIN_RESET); // 低电平
        TIM6_DelayUs(60);  // 脉冲间隔
        HAL_GPIO_WritePin(z_PU_PORT, z_PU_PIN, GPIO_PIN_SET);   // 高电平
        TIM6_DelayUs(80); // 脉冲间隔
      }
    
  
}

/**
 * @brief  每20ms内测试低电平次数，当次数连续达到五次以上判定为 触发了霍尔传感器，停止转轮转动
 */
bool IsHallTriggered(){
    int count = 0;
    for(int i = 0; i < 20; i++){
        if(HAL_GPIO_ReadPin(Hallsensor_PORT, Hallsensor_PIN_hall) == GPIO_PIN_RESET)
            count++;
        TIM6_DelayUs(3); // 脉冲间隔

    }
    return (count >= 5);
}
/**
 * @brief  驱动滤光片转轮
 */
void RotateFilterWheel() {

  printf("syz\r\n");
  int pulseNum=0;
  HAL_GPIO_WritePin(z_DR_PORT, z_DR_PIN, GPIO_PIN_SET);  // 设置旋转方向（根据实际方向调整）
  while(true){
     // 生成一个步进脉冲
        HAL_GPIO_WritePin(z_PU_PORT, z_PU_PIN, GPIO_PIN_RESET); // 低电平
        TIM6_DelayUs(8);  // 脉冲间隔
        HAL_GPIO_WritePin(z_PU_PORT, z_PU_PIN, GPIO_PIN_SET);   // 高电平
        TIM6_DelayUs(10); // 脉冲间隔
        //printf("%d\n",HAL_GPIO_ReadPin(Hallsensor_PORT,Hallsensor_PIN_hall));
      
        if(IsHallTriggered()){
          
            ForwardRotate(3); //手动校准系统性误差，转轮归正
            break; // 真正触发
        }
  }

  
}
/**
 * @brief  发出脉冲 pulse_x 和 pulse_y
 */
void move_stepper_motor(int axis) {

  
      if (axis == 1) {
          // 发出X轴脉冲
         HAL_GPIO_WritePin(X_PU_PORT, X_PU_PIN, GPIO_PIN_RESET); // 管脚复位，置0，将X轴脉冲引脚置为低电平，开始产生一个脉冲的下降沿
         TIM6_DelayUs(8);  // 延时8毫秒，保持低电平，确保脉冲宽度足够让步进电机识别
         HAL_GPIO_WritePin(X_PU_PORT, X_PU_PIN, GPIO_PIN_SET);   //管脚电平置1，将X轴脉冲引脚置为高电平，完成一个脉冲周期
         TIM6_DelayUs(10); // 延时10毫秒，为下一次脉冲留出间隔
        
      } else if (axis == 2) {
          // 发出Y轴脉冲
         HAL_GPIO_WritePin(Y_PU_PORT, Y_PU_PIN, GPIO_PIN_RESET);// 管脚复位，置0        
         TIM6_DelayUs(8); 
         HAL_GPIO_WritePin(Y_PU_PORT, Y_PU_PIN,GPIO_PIN_SET); // 管脚电平置1
         TIM6_DelayUs(10);
         
      }
}
/**
 * @brief  按照xy坐标距离差,移动到下一个目标孔位
 */
void moveDistanceXY(double x_distance, double y_distance) {
      
      //printf("x_distance: %f, y_distance: %f\n", x_distance, y_distance);// 打印输入参数

      // 计算方向
      int x_dir = (x_distance >= 0) ? GPIO_PIN_RESET : GPIO_PIN_SET;
      int y_dir = (y_distance >= 0) ? GPIO_PIN_SET : GPIO_PIN_RESET;
      //printf("x_dir: %d, y_dir: %d\n", x_dir, y_dir);
      //printf("fabs(x_distance) = %f\n", fabs(x_distance));
      //printf("fabs(y_distance) = %f\n", fabs(y_distance));
      // 计算脉冲数
      double x_temp = fabs(x_distance) * PULSES_PER_CM;
      double y_temp = fabs(y_distance) * PULSES_PER_CM;
      unsigned long x_pulses = (unsigned long)(x_temp + 0.5);
      unsigned long y_pulses = (unsigned long)(y_temp + 0.5);
      //printf("x_pulses: %lu, y_pulses: %lu\n", x_pulses, y_pulses);

      // 设置方向
      HAL_GPIO_WritePin(X_DR_PORT, X_DR_PIN, x_dir);
      HAL_GPIO_WritePin(Y_DR_PORT, Y_DR_PIN, y_dir);

      // 逐步步进 X 轴和 Y 轴
      while (x_pulses > 0 || y_pulses > 0) {
          if (x_pulses > 0) {
              move_stepper_motor(1);
              x_pulses--;
          }
          if (y_pulses > 0) {
              move_stepper_motor(2);
              y_pulses--;
          }
      }
      //TIM6_DelayUs(500000);  // 停留 0.5s 表示已扫描该孔
}


/**
 * @brief  X轴先移动 4mm，再让 Y轴固定移动 285mm
 */
void PopPlate(void)
{
     xyReset();//电机归位
    double x_distance = 1.0;   // cm
    double y_distance = 28.5;  // cm

    /* -------- X 轴方向，移动 4mm -------- */
    HAL_GPIO_WritePin(X_DR_PORT, X_DR_PIN, GPIO_PIN_SET); 
    unsigned long x_pulses =(unsigned long)(fabs(x_distance) * PULSES_PER_CM + 0.5);
    while (x_pulses--)
    {
        move_stepper_motor(1);   // 1 表示 X 轴
    }

    
    /* ================= Y 轴方向，移动 285mm ================= */
    HAL_GPIO_WritePin(Y_DR_PORT, Y_DR_PIN, GPIO_PIN_SET); 
    unsigned long y_pulses =(unsigned long)(fabs(y_distance) * PULSES_PER_CM + 0.5);

    while (y_pulses--)
    {
        move_stepper_motor(2);   // 2 表示 Y 轴
    }
}
void LoadPlate(void)
{
  
    xyReset();//先归位
    double x_distance = -9.3;   // cm
    double y_distance = 11.2;  // cm

    /* -------- X 轴方向，移动 1mm -------- */
    HAL_GPIO_WritePin(X_DR_PORT, X_DR_PIN, GPIO_PIN_SET); // X轴朝归位方向
    unsigned long x_pulses =(unsigned long)(fabs(x_distance) * PULSES_PER_CM + 0.5);
    while (x_pulses--)
    {
        move_stepper_motor(1);   // 1 表示 X 轴
    }

    
    /* ================= Y 轴方向，移动 15cm ================= */
    HAL_GPIO_WritePin(Y_DR_PORT, Y_DR_PIN, GPIO_PIN_SET); // Y轴朝归位方向
    unsigned long y_pulses =(unsigned long)(fabs(y_distance) * PULSES_PER_CM + 0.5);

    while (y_pulses--)
    {
        move_stepper_motor(2);   // 2 表示 Y 轴
    }
}

/**
 * @brief  让步进电机移动十万个脉冲，测量步进了多少距离
 */
/*void move100000pulses(){
//    unsigned long i;
//
//  //  printf("Stepper calibration start: 100000 pulses\r\n");
//
//    // 1. 设置 X 轴方向（根据你机械结构调整）
//    HAL_GPIO_WritePin(X_DR_PORT, X_DR_PIN, GPIO_PIN_RESET);  // 正方向
//
//    // 2. 发出 100000 个脉冲
//    for (i = 0; i < 100000; i++)
//    {
//        HAL_GPIO_WritePin(X_PU_PORT, X_PU_PIN, GPIO_PIN_RESET);
//        TIM6_DelayUs(8);
//
//        HAL_GPIO_WritePin(X_PU_PORT, X_PU_PIN, GPIO_PIN_SET);
//        TIM6_DelayUs(10);
//    }
*/

/**
 * @brief  X 、Y轴电机归位
 */
void xyReset(){

   /* ================== Y 轴归位 ================== */
    HAL_GPIO_WritePin(Y_DR_PORT, Y_DR_PIN, GPIO_PIN_RESET); // Y轴朝归位方向

    while (GPIOE->IDR & (1 << 15))  // Y限位未触发
    {
        HAL_GPIO_WritePin(Y_PU_PORT, Y_PU_PIN, GPIO_PIN_RESET);
        TIM6_DelayUs(8);
        HAL_GPIO_WritePin(Y_PU_PORT, Y_PU_PIN, GPIO_PIN_SET);
        TIM6_DelayUs(10);
    }
  /* ================== X 轴归位 ================== */
    HAL_GPIO_WritePin(X_DR_PORT, X_DR_PIN, GPIO_PIN_RESET); // X轴朝归位方向

    while (GPIOE->IDR & (1 << 11))  // X限位未触发
    {
        HAL_GPIO_WritePin(X_PU_PORT, X_PU_PIN, GPIO_PIN_RESET);
        TIM6_DelayUs(8);
        HAL_GPIO_WritePin(X_PU_PORT, X_PU_PIN, GPIO_PIN_SET);
        TIM6_DelayUs(10);
    }
  
   firstScan = true;
   last_x = 0, last_y = 0;  // 上次扫描结束时的列、行

}


double calculateDistance(int x1, int y1, int x2, int y2) {
    double dx = (x2 - x1) * HOLE_SPACING;
    double dy = (y2 - y1) * HOLE_SPACING;
    return sqrt(dx * dx + dy * dy);
}
/**
 * @brief  通过X/Y轴对角线移动模拟振荡器效果
 */
void shakePlate(void)
{
  if (firstScan) {
        LoadPlate();
  
       for (int i = 0; i < 60; i++) {//// 在XY对角线方向以0.1cm振幅晃动15次，每次间隔2us
      //printf("move\n");

            moveDistanceXY(0.05, 0.05);
            TIM6_DelayUs(2);
            moveDistanceXY(-0.05, -0.05);
            TIM6_DelayUs(3);
       }
    }
}




/**
 * @brief  扫描选中的孔位
 */
void scanAllHoles(int holes[][2], int hole_count) {
    //printf("hole_count: %d\n", hole_count);
    if (firstScan) {
        // 第一次：归位并移动到 A1
        xyReset();
        last_x = 0; last_y = 0;
        moveDistanceXY(HOME_OFFSET_X, HOME_OFFSET_Y);
        firstScan = false;
    } else {
        // 后续扫描：如果想从上次结束点继续，什么都不做；
        // 如果想每次都回到 A1 扫描，可启用下面两行：
        // moveDistanceXY((last_x - 0) * HOLE_SPACING, (last_y - 0) * HOLE_SPACING); // 回 A1
        // last_x = last_y = 0;
    }

    int prev_x = last_x;
    int prev_y = last_y;

    for (int i = 0; i < hole_count; i++) {
        int tgt_col = holes[i][1];  // X 轴步进（列）
        int tgt_row = holes[i][0];  // Y 轴步进（行）

        double dx = (prev_x - tgt_col) * HOLE_SPACING;
        double dy = (tgt_row - prev_y) * HOLE_SPACING;
        moveDistanceXY(dy, dx);
    
        
        // 先跑一轮但不算(必要性代码，否则测得的数据不稳定忽高忽低）
        ADC_DMA_Start(adc_dma_buffer, SAMPLES_COUNT);
        HAL_Delay(500);   // 等 DMA 跑完
        HAL_ADC_Stop_DMA(&hadcx);
        __HAL_ADC_DISABLE(&hadcx);
        hadcx.Instance->CR2 &= ~ADC_CR2_DMA; // 清除 DMA 使能位

        
        SolutionIntensity_StartSample();   // 到位后采样，且只采样只测试一次
        HAL_Delay(500);  // 停顿 0.5秒

        
        
       // printf("Scan hole: C:%d, R:%d\n", tgt_col, tgt_row);
        

        prev_x = tgt_col;
        prev_y = tgt_row;
    }

    // 更新上次结束位置
    last_x = prev_x;
    last_y = prev_y;
}





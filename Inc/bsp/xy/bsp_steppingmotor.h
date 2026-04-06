#ifndef _bsp_steppingmotor_H
#define _bsp_steppingmotor_H

#include "stm32f4xx_hal.h"

/*  steppingmotor控制时钟端口、引脚定义  X轴*/
#define X_PU_PORT         GPIOE
#define X_PU_PIN          GPIO_PIN_10

#define X_DR_PORT         GPIOE
#define X_DR_PIN          GPIO_PIN_9

#define X_MF_PORT         GPIOE
#define X_MF_PIN          GPIO_PIN_8

/*  steppingmotor控制时钟端口、引脚定义 Y轴*/
#define Y_PU_PORT         GPIOE
#define Y_PU_PIN          GPIO_PIN_14

#define Y_DR_PORT         GPIOE
#define Y_DR_PIN          GPIO_PIN_13

#define Y_MF_PORT         GPIOE
#define Y_MF_PIN          GPIO_PIN_12  //（如 GPIO_PIN_6 表示PC6）
/*  滤光片steppingmotor控制时钟端口、引脚定义 */
#define z_PU_PORT         GPIOG
#define z_PU_PIN          GPIO_PIN_7

#define z_DR_PORT         GPIOG
#define z_DR_PIN          GPIO_PIN_6

#define z_MF_PORT         GPIOG
#define z_MF_PIN          GPIO_PIN_5

// bsp_steppingmotor.h
extern TIM_HandleTypeDef htim2; // 声明定时器句柄
void steppingmotor_Init(void);

void xyReset();
//void move_stepper_motor(int axis);
//void moveDistanceXY(x_distance,y_distance);
//void takeSample(unsigned long x_distance,unsigned long y_distance);

void scanAllHoles(int holes[][2], int hole_count);
void RotateFilterWheel();
void shakePlate(void);
void PopPlate(void);
void LoadPlate(void);
#endif


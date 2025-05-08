#ifndef __TASK_CONTROLLER_H__
#define __TASK_CONTROLLER_H__
#include "main.h"
#include "task_gyro.h"
#include "task_joystick.h"

/* 
    Joystick speed comes in with values -32 - 32 
    - the joystick speed will be multiplied by SPEED_FACTOR to get the value closer to 100
*/
#define SPEED_FACTOR 3
/*
   This value will divide the angle to produce "steps" where a gyro value of ANGLE_ERROR represents 1 step
   - 1 step will produce a difference of 2 between the right and left motor (ex: right = speed + 1, left = speed - 1)
*/
#define ANGLE_ERROR 3

#define RAWANGLE_TO_ANGLE(x) ((x) / (10000))
#define RAWJOYSTICK_TO_JOYSTICK(x) ((((x) / (1000)) * 1000) / 320)
#define JOYSTICK_TO_SPEED(x) ((x))
#define ANGLE_TO_SPEEDDIF(x) ( ((x) / (ANGLE_ERROR)) * (4))

extern int32_t controller_angle;
extern int16_t car_speed;
extern SemaphoreHandle_t Semaphore_ControllerVals;
extern volatile bool RESET_ANGLE;

typedef enum
{
    CONTROLLER_OFF = 0,
    CONTROLLER_ON = 1
}controller_message_type_t;

int16_t get_motor_speeds(int32_t angle, int16_t speed);
int8_t saturate_to_8(int16_t x);
void task_controller_init(void);
#endif
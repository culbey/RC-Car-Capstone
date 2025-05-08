#ifndef __TASK_DCMOT_H__
#define __TASK_DCMOT_H__

#include "main.h"
#include "client.h"

#define PIN_DRIVE1		P10_2
#define PIN_DRIVE2      P10_0

#define PIN_2DRIVE1    P9_2
#define PIN_2DRIVE2    P10_4

#define MOTOR_FREQ     5000
#define DUTY_STOP      0xFF

#define MOTOR_LIMIT     100
#define MOTOR_MAX_CHANGE 50

typedef enum
{
    DRIVE_OFF = 0x00,               // 0
    DRIVE_FORWARD_SLOW = 0x19,      // 25
    DRIVE_FORWARD_MEDIUM = 0x32,    // 50
    DRIVE_FORWARD_FAST = 0x5A,      // 90
    DRIVE_BACKWARDS_SLOW = 0xE7,    // -25
    DRIVE_BACKWARDS_MEDIUM = 0xCE,  // -50
    DRIVE_BACKWARDS_FAST = 0xA6     // -90
} dcmot_message_type_t;


typedef struct {
    int8_t right_duty;
    int8_t left_duty;
} pwm_type_t;

extern EventGroupHandle_t sensor_event_group;
extern QueueHandle_t q_dcmot;
void task_dcmot_init(void);

#endif
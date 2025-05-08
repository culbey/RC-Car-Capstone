#ifndef __TASK_MOTOR_H__
#define __TASK_MOTOR_H__

#include "main.h"

#define PIN_DIR     P9_0
#define PIN_STEP    P9_1


typedef enum
{
    BACKWARD = 0,
    FORWARD = 1
} motor_message_type_t;

extern QueueHandle_t q_motor;
void task_motor_init(void);

#endif

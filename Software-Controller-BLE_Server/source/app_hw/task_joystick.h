#ifndef __TASK_JOYSTICK_H__
#define __TASK_JOYSTICK_H__
#include "main.h"

//var to access y pos value
extern uint16_t joystick_y_pos;
extern SemaphoreHandle_t Semaphore_Joystick;
typedef enum
{
    JOYSTICK_OFF = 0,
    JOYSTICK_ON = 1
} joystick_message_type_t;

#define PIN_ANALOG_JOY_Y  P10_5

#define JOYSTICK_THRESH_Y_UP_2P475V     0xBFFF
#define JOYSTICK_THRESH_Y_DOWN_0P825V   0x4000

typedef enum {
    JOYSTICK_POS_CENTER,
    JOYSTICK_POS_UP,
    JOYSTICK_POS_DOWN
}joystick_position_t;

void task_joystick_init(void);

void joystick_init(void);

uint16_t joystick_read_y(void);

joystick_position_t joystick_get_pos(void);

void joystick_print_pos(joystick_position_t position);
#endif
#include "main.h"
#include "task_gyro.h"
#include "task_joystick.h"
#include "task_controller.h"

#define PIN_RST_LCD P9_2
#define PIN_PB_OUT  P9_3

#define LCD_SUB_ADDR        (0x78 >> 1)
#define LCD_DATA_SEND       0x40
#define LCD_COMSEND         0x00
#define LCD_DATASEND        0x40
#define LCD_LINE1_ADDR      0x80
#define LCD_LINE2_ADDR      0xC0
#define LCD_NOP             0x00
#define LCD_FUNC_2LINE      0x38    // 8-bit interface mode
#define LCD_FUNC_BIGTXT     0x34    // 1 line, bigger font
#define LCD_ISTABLE_0       0x00    
#define LCD_ISTABLE_1       0x01
#define LCD_ISTABLE_2       0x02
#define LCD_BIAS_1_5        0x14
#define LCD_BIAS_1_4        0x1C
#define LCD_CONTRAST_CNTRL  0x78
#define LCD_POWER_CNTRL     0x5E
#define LCD_FOLLOWER_CNTRL  0x6D

#define LCD_SET_CURSOR      0x80
#define LCD_DISP_ON         0x0C
#define LCD_CURSOR_ON       0x02
#define LCD_CURSOR_BLINK    0x01
#define LCD_CLEAR           0x01

#define LCD_CURSOR_RIGHT    0x06


void no_wall(void);
void wall(void);
void task_lcd_init(void);
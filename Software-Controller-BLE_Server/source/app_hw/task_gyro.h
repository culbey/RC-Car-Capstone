#ifndef __TASK_GYRO_H__
#define __TASK_GYRO_H__

#include "main.h"

#define IAM20380_SUBORDINATE_ADDR 0x68 // b1101000
//D0, D1, 68, 69

//declarations needed for access in task_controller
extern int16_t gyro_z_value;
extern SemaphoreHandle_t Semaphore_Gyro;
int16_t read_gyro_value();
typedef enum
{
    GYRO_OP_READ,
    GYRO_OP_WRITE,
    GYRO_OP_INVALID,
} gyro_operation_t;

typedef enum
{
    GYRO_ADDR_INVALID = 0xFF,
    CONFIG_REG = 0x1A,
    GYRO_CONFIG = 0x1B,
    LOW_POWER_CONFIG = 0x1C,
    FIFO_EN = 0x1D,
    INT_PIN_CFG = 0x37,
    INT_ENABLE = 0x38,
    INT_STATUS = 0x39,
    GYRO_XOUT_H = 0x43,
    GYRO_XOUT_L = 0x44,
    GYRO_YOUT_H = 0x45,
    GYRO_YOUT_L = 0x46,
    GYRO_ZOUT_H = 0x47,
    GYRO_ZOUT_L = 0x48,
    USER_CTRL = 0x6A,
    PWR_MGMT_1 = 0x6B,
    PWR_MGMT_2 = 0x6C,
    SMPLRT_DIV = 0x19,
    TEMP_OUT_H = 0x41,
    TEMP_OUT_L = 0x42
} gyro_reg_addr_t;

typedef struct
{
    uint16_t x_value;
    uint16_t y_value;
    uint16_t z_value;
} gyro_packet_t;

typedef enum
{
    GYRO_OFF = 0,
    GYRO_ON = 1
} gyro_message_type_t;

extern QueueHandle_t q_gyro;
void task_gyro_init(void);

#endif
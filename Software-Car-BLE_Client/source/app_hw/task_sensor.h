#ifndef __TASK_SENSOR_H__
#define __TASK_SENSOR_H__

#include "main.h"
#include "i2c.h"
#include "VL53L4CD_ULD_V2.2.2/VL53L4CD_ULD_Driver/VL53L4CD_api.h"
#include "client.h"
#include "task_dcmot.h"

typedef enum
{
    SENSOR_OFF = 0,
    SENSOR_ON = 1
} sensor_message_type_t;

void task_sensor_init(void);

extern TaskHandle_t Sensor_Task;

#endif
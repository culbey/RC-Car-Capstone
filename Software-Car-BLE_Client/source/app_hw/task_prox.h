#ifndef __TASK_PROX_H__
#define __TASK_PROX_H__

#include "main.h"
#include "i2c.h"
#include "VL53L4CD_ULD_V2.2.2/VL53L4CD_ULD_Driver/VL53L4CD_api.h"


#define PIN_SCL     PX_Y
#define PIN_SDA     PX_Z


extern QueueHandle_t q_prox;
void task_prox_init(void);

#endif
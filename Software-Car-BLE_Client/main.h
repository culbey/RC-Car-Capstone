/**
 * @file ece353.h
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-08-15
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#ifndef __MAIN_H__
#define __MAIN_H__

/* Include Infineon BSP Libraries */
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"

/* Include Standard C Libraries*/
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

/* FreeRTOS Includes */
#include <FreeRTOS.h>
#include <event_groups.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>
#include "source/FreeRTOSConfig.h"
#include "source/FreeRTOS_CLI.h"

/* Include Project Specific Files */
#include "source/app_hw/i2c.h"
#include "source/app_hw/task_console.h"
#include "client.h"
#include "source/app_hw/task_motor.h"
#include "source/app_hw/task_prox.h"
#include "source/app_hw/task_sensor.h"
#include "source/app_hw/task_dcmot.h"
#include "source/VL53L4CD_ULD_V2.2.2/Platform/platform.h"

#endif
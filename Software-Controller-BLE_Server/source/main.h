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
#include "FreeRTOSConfig.h"
#include "FreeRTOS_CLI.h"

/* Include Project Specific Files */
#include "i2c.h"
#include "task_console.h"
#endif
#include "task_lcd.h"


char write_msg[21];
QueueHandle_t q_lcd_write;
TaskHandle_t Update_Display_Task;
TaskHandle_t Button_Task;

volatile int x = -1;

/* FreeRTOS CLI Handler for the 'imu_accel' command */
static BaseType_t cli_handler_lcd_write(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString)
{

    const char *pcParameter;
    BaseType_t xParameterStringLength, xReturn;

    /* Remove compile time warnings about unused parameters, and check the
    write buffer is not NULL.  NOTE - for simplicity, this example assumes the
    write buffer length is adequate, so does not check for buffer overflows. */
    (void)pcCommandString;
    (void)xWriteBufferLen;
    configASSERT(pcWriteBuffer);

    /* Obtain the parameter string. */
    pcParameter = FreeRTOS_CLIGetParameter(
        pcCommandString,        /* The command string itself. */
        1,                      /* Return the 1st parameter. */
        &xParameterStringLength /* Store the parameter string length. */
    );
    /* Sanity check something was returned. */
    configASSERT(pcParameter);

    if (xParameterStringLength > 20) {
        /* Return a string indicating an invalid parameter. */
        memset(pcWriteBuffer, 0x00, xWriteBufferLen);
        sprintf(pcWriteBuffer, "Error... string too large. Needs to be 20 characters or less\n\r");
        xReturn = pdFALSE;
        return xReturn;
    }


    strncpy(write_msg, pcParameter, 20);
    write_msg[21] = '\0';

    char *ptr = write_msg;
    //  Send the message to the lcd write task
    x = -1;
    xQueueSendToBack(q_lcd_write, &ptr, portMAX_DELAY);

    memset(pcWriteBuffer, 0, xWriteBufferLen);

    /* Indicate that the command has completed */
    xReturn = pdFALSE;

    return xReturn;
}

void nextline(void)
{
	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);

    uint8_t write_data1[2] = { LCD_COMSEND, LCD_LINE2_ADDR };
	cy_rslt_t rslt = cyhal_i2c_master_write(&i2c_master_obj, LCD_SUB_ADDR, write_data1, 2, 0, true);
	CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    xSemaphoreGive(Semaphore_I2C);
}


void lcd_init(void) {
    cy_rslt_t rslt;

    xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);

	uint8_t write_data1[10] = { LCD_NOP, LCD_FUNC_2LINE, LCD_FUNC_2LINE | LCD_ISTABLE_1, LCD_BIAS_1_5, LCD_CONTRAST_CNTRL, LCD_POWER_CNTRL, LCD_FOLLOWER_CNTRL, LCD_DISP_ON, LCD_CLEAR, LCD_CURSOR_RIGHT };
	rslt = cyhal_i2c_master_write(&i2c_master_obj, LCD_SUB_ADDR, write_data1, 10, 0, true);

	CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    cyhal_system_delay_us(10);
    xSemaphoreGive(Semaphore_I2C);

}

void ClearDisplay(void) {
    xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);

    uint8_t write_data1[2] = { LCD_COMSEND, LCD_CLEAR };
	cy_rslt_t rslt = cyhal_i2c_master_write(&i2c_master_obj, LCD_SUB_ADDR, write_data1, 2, 0, true);
	CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    xSemaphoreGive(Semaphore_I2C);
}

void ClearLine2(void) {
    nextline();

    cy_rslt_t rslt;

    xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);

    uint8_t write_data[21];
    write_data[0] = 0x40;               // datasend
    for (int i = 1; i <= 20; i++) {     // text
        write_data[i] = '\0';
    }
	rslt = cyhal_i2c_master_write(&i2c_master_obj, LCD_SUB_ADDR, write_data, 21, 0, true);
	CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    cyhal_system_delay_ms(2);
    xSemaphoreGive(Semaphore_I2C);

    nextline();
}

void Show(unsigned char *text)
{

    cy_rslt_t rslt;

    xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);

    uint8_t write_data[21];
    write_data[0] = 0x40;       // datasend
    for (int i = 1; i <= 21; i++) {     // text
        write_data[i] = *text;
        text++;
    }
	rslt = cyhal_i2c_master_write(&i2c_master_obj, LCD_SUB_ADDR, write_data, 21, 0, true);
	CY_ASSERT(rslt == CY_RSLT_SUCCESS);

    cyhal_system_delay_us(10);
    xSemaphoreGive(Semaphore_I2C);
}



void task_lcd_write(void *param) {

    // reset LCD
    cyhal_gpio_write(PIN_RST_LCD, 0);
    cyhal_system_delay_ms(10);
    cyhal_gpio_write(PIN_RST_LCD, 1);
    cyhal_system_delay_ms(10);

    /* Suppress warning for unused parameter */
    (void)param;

    lcd_init();
    cyhal_system_delay_ms(2);

    char *req;
    char text[20] = {'\0', '\0', '\0', 'C', 'r', 'a', 's', 'h', '\0', 'O', 'u', 't', '\0', 'C', 'a', 'r', '\0', '\0', '\0', '\0'};

    char *p = text;
    Show((unsigned char *) p);
    nextline();

    for (;;)
    {
        // Check the Queue
        xQueueReceive(q_lcd_write, &req, portMAX_DELAY);

        for (int i = 0; i < 20; i++) {
            if (req == NULL) break;
            text[i] = *req;
            req++;
        }

        ClearLine2();
        Show((unsigned char *) text);

        for (int i = 0; i < 20; i++) {
            text[i] = '\0';
        }
        
    }
}

/* The CLI command definition command */
static const CLI_Command_Definition_t lcd_write =
    {
        "lcd_write",                      /* command text */
        "\r\nlcd_write\r\n", /* command help text */
        cli_handler_lcd_write,            /* The function to run. */
        1                             /* The user can enter 1 parameters */
};


TimerHandle_t flash_timer;
char wall_display[21] = {"        WALL"};
char *wall_ptr = wall_display;
bool wall_seen = false;
void wall(void) {
    if (wall_seen) return;
    x = -1;
    xQueueSendToBack(q_lcd_write, &wall_ptr, portMAX_DELAY);
    wall_seen = true;
    xTimerStart(flash_timer, portMAX_DELAY);
}

char no_wall_display[21] = {"    Wall Avoided"};
char *no_wall_ptr = no_wall_display;
void no_wall(void) {
    if (!wall_seen) return;
    x = -1;
    xTimerStop(flash_timer, portMAX_DELAY);
    wall_ptr = wall_display;        // ensure we reset the ptr to wall display
    xQueueSendToBack(q_lcd_write, &no_wall_ptr, portMAX_DELAY);
    wall_seen = false;

}

void flash_timer_callback(TimerHandle_t xTimer) {
    
    wall_ptr = (wall_ptr == wall_display) ? NULL : wall_display;
    xQueueSendToBack(q_lcd_write, &wall_ptr, portMAX_DELAY);
    
}


/* Interrupt handler callback function */
char displays[5][21] = {"", "      Gyro: 000", "   Joystick: 000", "  L: 000    R: 000"};
int GYRO_IND = 1;
int JOYSTICK_IND = 2;
int MOTORS_IND = 3;
int CLOCK_IND = 0;
int NUM_DISPLAYS = 4;

// This task updates display values and then signals for the new values to be displayed
void task_update_dispVals(void *param) {
    /* Suppress warning for unused parameter */
    (void)param;

    /* Time initialization */
    cyhal_rtc_t rtc_obj;
    cy_rslt_t rslt = cyhal_rtc_init(&rtc_obj);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt);

    // initialize time
    struct tm time;
    const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char month_str[4];
    int day, year, hour, min, sec;

    sscanf(__DATE__, "%3s %d %d", month_str, &day, &year);
    sscanf(__TIME__, "%d:%d:%d", &hour, &min, &sec);

    int month = (strstr(months, month_str) - months) / 3;

    time.tm_year = year - 1900;
    time.tm_mon = month;
    time.tm_mday = day;
    time.tm_hour = hour;
    time.tm_min = min;
    time.tm_sec = sec;
    
    rslt = cyhal_rtc_write(&rtc_obj, &time);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt);

    /* Intitialization for Gyro and Joystick updates */
    int32_t angle;
    int16_t speed;
    int8_t left, right;

    while (1) {

        // Clock is being used as first display, so we do not have to take the controller vals semaphore
        // when the values are not being used to display
        if ( x < 0 || x == CLOCK_IND || x == NUM_DISPLAYS - 1) { // UPDATE CLOCK VAL
            rslt = cyhal_rtc_read(&rtc_obj, &time);
            CY_ASSERT(CY_RSLT_SUCCESS == rslt);

            if (time.tm_hour == 0) {
                hour = 12;
            } else if (time.tm_hour > 12) {
                hour = time.tm_hour % 12;
            } else {
                hour = time.tm_hour;
            }

            snprintf(displays[CLOCK_IND], sizeof(displays[CLOCK_IND]), "  %.3s %02d  %02d:%02d:%02d", months + (time.tm_mon * 3), time.tm_mday, hour, time.tm_min, time.tm_sec);
        }

        if (x >= 0) {

            xSemaphoreTake(Semaphore_ControllerVals, portMAX_DELAY);
            angle = controller_angle;
            speed = car_speed;
            xSemaphoreGive(Semaphore_ControllerVals);

            angle = RAWANGLE_TO_ANGLE(angle);
            speed = RAWJOYSTICK_TO_JOYSTICK(speed);

            if (x == GYRO_IND || x == GYRO_IND - 1) {
                snprintf(displays[GYRO_IND], sizeof(displays[GYRO_IND]), "      Gyro: %03d", (int8_t) angle);
            }
            
            if (x == JOYSTICK_IND || x == JOYSTICK_IND - 1) {
                snprintf(displays[JOYSTICK_IND], sizeof(displays[JOYSTICK_IND]), "   Joystick: %03d", speed);
            }
            
            if (x == MOTORS_IND || x == MOTORS_IND - 1) {
                
                speed = get_motor_speeds(angle, speed);
                left = (int8_t) (speed >> 8);
                right = (int8_t) (speed & 0x00FF);

                // if ((left & 0x8000) ^ (right & 0x8000)) { // check if the motor speeds have different signs

                // }
                
                if (left <= -100) {
                    snprintf(displays[MOTORS_IND], sizeof(displays[MOTORS_IND]), "  L: %03d  R: %03d ", left, right);
                } else {
                    snprintf(displays[MOTORS_IND], sizeof(displays[MOTORS_IND]), "  L: %03d   R: %03d ", left, right);
    
                }
    
            }

        }


        if (x >= 0) xTaskNotifyGive(Update_Display_Task);
        
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

volatile bool BUTTON_READY = true;

TimerHandle_t hold_timer;
volatile bool TIMER_EXPIRED = false;
volatile bool SHORT_DEBOUNCE = false;
void hold_timer_callback(TimerHandle_t xTimer) {
    // This runs only if button is held for full 2 seconds
    if (cyhal_gpio_read(PIN_PB_OUT) == false && x == GYRO_IND) RESET_ANGLE = true;
    //RESET_ANGLE = true;
    TIMER_EXPIRED = true;
}

void lcd_button_ISR(void *handler_arg, cyhal_gpio_event_t event)
{
    //CY_UNUSED_PARAMETER(event);
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    // if we still have an unhandled button press, return
    if (BUTTON_READY == false || wall_seen) {
        return;
    }


    BUTTON_READY = false; // ensure we have a way to prevent debounce after a button press or release
    vTaskNotifyGiveFromISR(Button_Task, &xHigherPriorityTaskWoken);

    if (x == GYRO_IND) {
        if (event == CYHAL_GPIO_IRQ_FALL) {
            SHORT_DEBOUNCE = true;
            TIMER_EXPIRED = false;
            xTimerStartFromISR(hold_timer, &xHigherPriorityTaskWoken);
            return;
        } else {
            SHORT_DEBOUNCE = false;
            if (TIMER_EXPIRED == true) {
                return;
            }

            xTimerStopFromISR(hold_timer, &xHigherPriorityTaskWoken);
        }
    } else if (event == CYHAL_GPIO_IRQ_RISE) {
        return;
    }


    x = (x + 1) % NUM_DISPLAYS;
    
    // if (x == 2) {
    //     xTaskNotifyGiveFromISR(Update_Display_Task, &xHigherPriorityTaskWoken);
    // }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

}

void task_button(void *param) {
    (void) param;

    for (;;) {

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (x >= 0) {
            xTaskNotifyGive(Update_Display_Task);
        }

        if (SHORT_DEBOUNCE) {
            vTaskDelay(pdMS_TO_TICKS(3));
        } else {
            vTaskDelay(pdMS_TO_TICKS(200));
        }

        BUTTON_READY = true;
    }

}

void task_display_screen(void *param) {
    /* Suppress warning for unused parameter */
    (void)param;
    
    char *disp_ptr;

    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        disp_ptr = displays[x];
        xQueueSendToBack(q_lcd_write, &disp_ptr, portMAX_DELAY);
        
    }
}

cyhal_gpio_callback_data_t callback_data;
void task_lcd_init(void) {

    /* init queues for sending imu data from task to cli handler */
    q_lcd_write = xQueueCreate(1, sizeof(char *)); 

    /* init LCD reset pin */
    cy_rslt_t rslt = cyhal_gpio_init(PIN_RST_LCD, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, 1);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt);

    /* init push button gpio pin input */
    rslt = cyhal_gpio_init(PIN_PB_OUT, CYHAL_GPIO_DIR_INPUT, CYHAL_GPIO_DRIVE_NONE, 1);
    CY_ASSERT(CY_RSLT_SUCCESS == rslt);

    callback_data.callback = lcd_button_ISR;
    /* Register callback function - gpio_interrupt_handler and pass the value global_count */
    cyhal_gpio_register_callback(PIN_PB_OUT, (cyhal_gpio_callback_data_t *) &callback_data);
    /* Enable falling edge interrupt event with interrupt priority set to 3 */
    cyhal_gpio_enable_event(PIN_PB_OUT, CYHAL_GPIO_IRQ_BOTH, 3, true);

    /* Initialize timer for long press to reset gyro */
    hold_timer = xTimerCreate(
        "HoldTimer",
        pdMS_TO_TICKS(2000),
        pdFALSE,      // One-shot
        NULL,
        hold_timer_callback
    );

    /* Initialize timer for flashing WALL */
    flash_timer = xTimerCreate(
        "FlashTimer",
        pdMS_TO_TICKS(300),
        pdTRUE,      // One-shot
        NULL,
        flash_timer_callback
    );

    /* Register the CLI command */
    FreeRTOS_CLIRegisterCommand(&lcd_write);

    /* Create the task that will control the status LED */
    xTaskCreate(
        task_lcd_write,
        "Task_LCD_write",
        configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 6,
        NULL
    );

    /* Create the task that will control the status LED */
    xTaskCreate(
        task_display_screen,
        "Task_Display_Screen",
        configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 6,
        &Update_Display_Task
    );

    /* Create the task that will control the status LED */
    xTaskCreate(
        task_update_dispVals,
        "Task_Update_Display_Values",
        configMINIMAL_STACK_SIZE * 3,
        NULL,
        configMAX_PRIORITIES - 6,
        NULL
    );

    /* Create the task that will control the status LED */
    xTaskCreate(
        task_button,
        "Task_Button",
        configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 6,
        &Button_Task
    );

}
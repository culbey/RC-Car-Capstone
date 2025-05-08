#include "task_joystick.h"

uint16_t joystick_y_pos = 0x8000;
SemaphoreHandle_t Semaphore_Joystick;

static void timer_callback_joystick(TimerHandle_t xTimer);
static void task_joystick(void *param);

static BaseType_t cli_handler_joystick(
    char *pcWritePuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString);

QueueHandle_t q_joystick;
TimerHandle_t joystick_timer;

static const CLI_Command_Definition_t xJoystick =
    {
        "joystick",
        "\r\n joystick<on|off>\r\n",
        cli_handler_joystick,
        1};

static cyhal_adc_t joystick_adc_obj;
//static cyhal_adc_channel_t joystick_adc_chan_x_obj;
static cyhal_adc_channel_t joystick_adc_chan_y_obj;
const cyhal_adc_channel_config_t channel_config = {.enable_averaging = false, .min_acquisition_ns = 220, .enabled = true};
const cyhal_adc_config_t joystic_config = {
    .continuous_scanning = true,
    .resolution = 12,
    .average_count = 1,
    .average_mode_flags = 0,
    .ext_vref_mv = 0,
    .vneg = CYHAL_ADC_VNEG_VREF,
    .vref = CYHAL_ADC_REF_VDDA_DIV_2,
    .ext_vref = NC,
    .is_bypassed = false,
    .bypass_pin = NC};

void joystick_init(void)
{
    cy_rslt_t rslt;

    /* Initialize ADC.*/
    rslt = cyhal_adc_init(&joystick_adc_obj, PIN_ANALOG_JOY_Y, NULL);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS); // If the initialization, halt the MCU

    /* Set the Reference Voltage to be VDDA */
    rslt = cyhal_adc_configure(&joystick_adc_obj, &joystic_config);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS); // If the initialization, halt the MCU

    /* Initialize X direction */
    // rslt = cyhal_adc_channel_init_diff(
    //     &joystick_adc_chan_x_obj,
    //     &joystick_adc_obj,
    //     PIN_ANALOG_JOY_X,
    //     CYHAL_ADC_VNEG,
    //     &channel_config);
    // CY_ASSERT(rslt == CY_RSLT_SUCCESS); // If the initialization, halt the MCU

    /* Initialize Y direction */
    rslt = cyhal_adc_channel_init_diff(
        &joystick_adc_chan_y_obj,
        &joystick_adc_obj,
        PIN_ANALOG_JOY_Y,
        CYHAL_ADC_VNEG,
        &channel_config);
    CY_ASSERT(rslt == CY_RSLT_SUCCESS); // If the initialization, halt the MCU
}

uint16_t joystick_read_y(void)
{
    uint16_t toReturn;
    xSemaphoreTake(Semaphore_Joystick, portMAX_DELAY);
    joystick_y_pos = cyhal_adc_read_u16(&joystick_adc_chan_y_obj);
    toReturn = joystick_y_pos;
    xSemaphoreGive(Semaphore_Joystick);
    return toReturn;
}

/**
 * @brief
 * Returns the current position of the joystick
 * @return joystick_position_t
 */
joystick_position_t joystick_get_pos(void)
{
    bool up = false;
    bool down = false;
    joystick_read_y();
    up = ((joystick_y_pos > JOYSTICK_THRESH_Y_UP_2P475V) ? true : false);
    down = ((joystick_y_pos < JOYSTICK_THRESH_Y_DOWN_0P825V) ? true : false);

    if (up)
    {
        return JOYSTICK_POS_UP;
    }
    else if (down)
    {
        return JOYSTICK_POS_DOWN;
    }

    return JOYSTICK_POS_CENTER;
}

/**
 * @brief
 * Prints out a string to the console based on parameter passed
 * @param position
 * The enum value to be printed.
 */
void joystick_print_pos(joystick_position_t position)
{
    switch (position)
    {
    case (JOYSTICK_POS_CENTER):
        task_print("center\n");
        break;
    case (JOYSTICK_POS_UP):
        task_print("up\n");
        break;
    case (JOYSTICK_POS_DOWN):
        task_print("down\n");
        break;
    }
}

static void timer_callback_joystick(TimerHandle_t xTimer)
{
    // read and output the voltage on X and Y
    // Need ADC conversion?
    task_print("Jostick: %d\n", (int16_t) (joystick_read_y() - 32768));
}

static void task_joystick(void *param)
{
    joystick_message_type_t message_type;
    (void)param;
    for (;;)
    {
        // Check the Queue.  If nothing was in the queue, we should return pdFALSE
        xQueueReceive(q_joystick, &message_type, portMAX_DELAY);
        if (message_type == JOYSTICK_OFF)
        {
            xTimerStop(joystick_timer, portMAX_DELAY);
        }
        else if (message_type == JOYSTICK_ON)
        {
            xTimerStart(joystick_timer, portMAX_DELAY);
        }
    }
}

static BaseType_t cli_handler_joystick(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString)
{
    const char *pcParameter;
    joystick_message_type_t joystick_type;
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
    /* Copy ONLY the parameter to pcWriteBuffer */
    memset(pcWriteBuffer, 0x00, xWriteBufferLen);
    strncat(pcWriteBuffer, pcParameter, xParameterStringLength);

    if (strcmp(pcWriteBuffer, "on") == 0)
    {
        joystick_type = JOYSTICK_ON;
    }
    else if (strcmp(pcWriteBuffer, "off") == 0)
    {
        joystick_type = JOYSTICK_OFF;
    }
    else
    {
        /* Return a string indicating an invalid parameter. */
        memset(pcWriteBuffer, 0x00, xWriteBufferLen);
        sprintf(pcWriteBuffer, "Error... invalid parameter\n\r");
        xReturn = pdFALSE;
        return xReturn;
    }

    xQueueSendToBack(q_joystick, &joystick_type, portMAX_DELAY);
    /* Nothing to return, so zero out the pcWriteBuffer. */
    memset(pcWriteBuffer, 0, xWriteBufferLen);
    /* Indicate that the command has completed */
    xReturn = pdFALSE;
    return xReturn;
}

void task_joystick_init(void)
{
    joystick_init();
    Semaphore_Joystick = xSemaphoreCreateBinary();
    xSemaphoreGive(Semaphore_Joystick);
    q_joystick = xQueueCreate(1, sizeof(joystick_message_type_t));
    FreeRTOS_CLIRegisterCommand(&xJoystick);

    joystick_timer = xTimerCreate(
        "Joystick Timer",
        pdMS_TO_TICKS(750),
        true,
        0,
        timer_callback_joystick);
    xTaskCreate(
        task_joystick,
        "Task Joystick",
        configMINIMAL_STACK_SIZE,
        NULL,
        configMAX_PRIORITIES - 6,
        NULL);
}
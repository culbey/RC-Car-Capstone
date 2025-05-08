#include "task_controller.h"

static void task_controller(void *param);

SemaphoreHandle_t Semaphore_ControllerVals;
int32_t controller_angle = 0;
int16_t car_speed = 0;
volatile bool RESET_ANGLE = false;

static BaseType_t cli_handler_controller(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString);

static const CLI_Command_Definition_t xController =
    {
        "controller",
        "\r\n controller <on|off>\r\n",
        cli_handler_controller,
        1};


int16_t get_motor_speeds(int32_t angle, int16_t speed) {
        int16_t left, right, dif, offset;

        // joystick up = forward
        // joystick down = reverse
        // mtr speed = joystick * 3

        if (speed == 0) {
            return 0x0000;
        }

        left = JOYSTICK_TO_SPEED(abs(speed));
        right = left;
        // gyro right = negative
        // gyro left = positive
        // right mtr = speed + gyro
        // left mtr = speed - gyro
        dif = ANGLE_TO_SPEEDDIF(angle);

        left -= dif;
        right += dif;

        offset = (abs(speed) + abs(dif)) - 100;
        if (offset > 0) {

            if (dif < 0) { // left is outside range of -100 - 100
                right -= offset;
            } else {    // right is outside range of -100 - 100
                left -= offset;
            }

        }
        
        if (speed < 0) {
            right *= -1;
            left *= -1;
        }

        return ((int16_t)((uint8_t)saturate_to_8(left)) << 8) | (uint8_t)saturate_to_8(right);
}
int8_t saturate_to_8(int16_t x) {
    if (x > 100) return 100;
    if (x < -100) return -100;
    return x;
}

static void task_controller(void *param)
{
    uint16_t joystick_pos;
    int16_t gyro_val;
    int16_t modified_joystick;
    int32_t modified_gyro;

    TickType_t ms = 2;
    // sensor favors negative values when stationary, so this is an addition to help bias towards more centered values (around 1/10 of interval)
    int bias = 18;
    // this is the smallest value that affects the steering. (values between -149 - 149 are ignored to reduce noise).
    // reducing this value means the steering will detect slower movements, but will have higher noise
    // increasing this value means that the steering not detect slower movements but will have less noise.
    // found that: 
    // - 100 seems to be a little too inaccurate because of noise.
    // - 150 - pretty accurate, detects slow movements but can still be thrown off by very slow movements
    // - 200 - 500 can be accurate as long as wheel is not moved too slowly
    int interval = 150;
    

    for(;;) {
        //run I2C reads for joystick and gyroscope

        joystick_pos = joystick_read_y(); //joystic_y_pos declared externally (uint16_t)
        gyro_val = read_gyro_value(); //gyro_z_value declared externally (short)
        

        modified_joystick = joystick_pos - 32768;
        modified_gyro = ((gyro_val + bias) / 20) * ms;

        xSemaphoreTake(Semaphore_ControllerVals, portMAX_DELAY);
        car_speed = modified_joystick;
        if (RESET_ANGLE) {
            controller_angle = 0;
            RESET_ANGLE = false;
        } else {
            controller_angle += modified_gyro;
        }
        xSemaphoreGive(Semaphore_ControllerVals);
        
        vTaskDelay(pdMS_TO_TICKS(ms));
    }
}

static BaseType_t cli_handler_controller(
    char *pcWriteBuffer,
    size_t xWriteBufferLen,
    const char *pcCommandString)
{
    const char *pcParameter;
    controller_message_type_t controller_type;
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
    //necessary?
    if (strcmp(pcWriteBuffer, "on") == 0)
    {
        controller_type = CONTROLLER_ON;
    }
    else if (strcmp(pcWriteBuffer, "off") == 0)
    {
        controller_type = CONTROLLER_OFF;
    }
    else
    {
        /* Return a string indicating an invalid parameter. */
        memset(pcWriteBuffer, 0x00, xWriteBufferLen);
        sprintf(pcWriteBuffer, "Error... invalid parameter\n\r");
        xReturn = pdFALSE;
        return xReturn;
    }

    //?xQueueSendToBack(q_joystick, &joystick_type, portMAX_DELAY);
    /* Nothing to return, so zero out the pcWriteBuffer. */
    memset(pcWriteBuffer, 0, xWriteBufferLen);
    /* Indicate that the command has completed */
    xReturn = pdFALSE;
    return xReturn;
}

void task_controller_init(void)
{

    Semaphore_ControllerVals = xSemaphoreCreateBinary();
	xSemaphoreGive(Semaphore_ControllerVals);

    //FreeRTOS_CLIRegisterCommand(&xController);

    xTaskCreate(
        task_controller,
        "Task Controller",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        configMAX_PRIORITIES - 1,
        NULL);
}
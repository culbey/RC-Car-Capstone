#include "task_bluetooth.h"

volatile bool PRINT_BLUETOOTH = false;

void task_bt_send_motor(void *param) {

    int32_t angle;
    int16_t speed;
    int8_t left, right;

    for (;;) {

        // check if we have a connection first and check if the client has subscribed to notifications
        if (0 == hello_sensor_state.conn_id || 0 == app_crash_out_car_motors_client_char_config[0]) {
            if (PRINT_BLUETOOTH) {
                task_print("-------------------------------\n");
                task_print("No connection or no connection subscribed to notifications\n\r");
                task_print("-------------------------------\n");
            }
            vTaskDelay(1000);
            continue;
        }
        /* Increment the last byte of the hello sensor notify value */
        xSemaphoreTake(Semaphore_ControllerVals, portMAX_DELAY);
        angle = controller_angle;
        speed = car_speed;
        xSemaphoreGive(Semaphore_ControllerVals);

        angle = RAWANGLE_TO_ANGLE(angle);
        speed = RAWJOYSTICK_TO_JOYSTICK(speed);

        speed = get_motor_speeds(angle, speed);
        left = (int8_t) (speed >> 8);
        right = (int8_t) (speed & 0x00FF);

        if (PRINT_BLUETOOTH) {
            task_print("-------------------------------\n");
            task_print("Left Motor: %d    ", left);
            task_print("Right Motor: %d\n", right);
            task_print("-------------------------------\n");
        }
        app_bt_gatt_set_motor_speeds(left, right);

        /* Remember how many messages we need to send */
        hello_sensor_state.num_to_send++;

        /* Connection up.
            * Send message if client registered to receive indication
            * or notification. After we send an indication wait for the
            * ack before we can send anything else */

        
        // task_print("No. to write: %d\n",
        //         hello_sensor_state.num_to_send);
        // task_print("flag_indication_sent: %d \n",
        //         hello_sensor_state.flag_indication_sent);

        while ((0 != hello_sensor_state.num_to_send) &&
                (FALSE == hello_sensor_state.flag_indication_sent))
        {
            hello_sensor_state.num_to_send--;
            app_bt_send_message();
        }

        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

static BaseType_t cli_handler_bluetooth(
	char *pcWriteBuffer,
	size_t xWriteBufferLen,
	const char *pcCommandString)
{
	BaseType_t xParameterStringLength, xReturn;
	const char *pcParameter;

	/* Remove compile time warnings about unused parameters, and check the
	write buffer is not NULL.  NOTE - for simplicity, this example assumes the
	write buffer length is adequate, so does not check for buffer overflows. */
	(void)pcCommandString;
	(void)xWriteBufferLen;
	configASSERT(pcWriteBuffer);

	/* Obtain the on/off string. */
	pcParameter = FreeRTOS_CLIGetParameter(
		pcCommandString,		/* The command string itself. */
		1,						/* Return the 1st parameter. */
		&xParameterStringLength /* Store the parameter string length. */
	);
	/* Sanity check something was returned. */
	configASSERT(pcParameter);

	/* Copy ONLY the parameter to pcWriteBuffer */
	memset(pcWriteBuffer, 0x00, xWriteBufferLen);
	strncat(pcWriteBuffer, pcParameter, xParameterStringLength);

	if (strcmp(pcWriteBuffer, "on") == 0)
	{
		PRINT_BLUETOOTH = true;
	}
	else if (strcmp(pcWriteBuffer, "off") == 0)
	{
		PRINT_BLUETOOTH = false;
	}
	else
	{
		/* Return a string indicating an invalid parameter. */
		memset(pcWriteBuffer, 0x00, xWriteBufferLen);
		sprintf(pcWriteBuffer, "Error... invalid parameter\n\r");
		xReturn = pdFALSE;
		return xReturn;
	}

	/* Nothing to return, so zero out the pcWriteBuffer. */
	memset(pcWriteBuffer, 0, xWriteBufferLen);

	/* Indicate that the command has completed */
	xReturn = pdFALSE;

	return xReturn;
}

static const CLI_Command_Definition_t cmd_bluetooth =
	{
		"bluetooth",					 /* command text */
		"\r\nbluetooth <on|off>\r\n", /* command help text */
		cli_handler_bluetooth,		 /* The function to run. */
		1						 /* The user can enter any number of parameters */
};

void task_bt_notify_init(void) {

    /* Register the CLI command */
	FreeRTOS_CLIRegisterCommand(&cmd_bluetooth);

    /* Create the task that will control the status LED */
    xTaskCreate(
        task_bt_send_motor,
        "Task_BT_Motor_Notify",
        configMINIMAL_STACK_SIZE * 2,
        NULL,
        configMAX_PRIORITIES - 6,
        NULL
    );

}
/**
 * @file gyro.c
 * @author Andrew Sabee (apmcavoy@wisc.edu)
 * @brief
 * @version 0.1
 * @date 2023-09-01
 *
 * @copyright Copyright (c) 2023
 *
 */
#include "task_gyro.h"

/******************************************************************************/
/* Function Declarations                                                      */
/******************************************************************************/
int16_t gyro_z_value = 0x0000;
SemaphoreHandle_t Semaphore_Gyro;

static void timer_callback_gyro(TimerHandle_t xTimer);
static void task_gyro(void *param);
static BaseType_t cli_handler_gyro(
	char *pcWriteBuffer,
	size_t xWriteBufferLen,
	const char *pcCommandString);

/******************************************************************************/
/* Global Variables                                                           */
/******************************************************************************/

QueueHandle_t q_gyro;
TimerHandle_t gyro_timer;
static const CLI_Command_Definition_t cmd_gyro =
	{
		"gyro",					 /* command text */
		"\r\ngyro <on|off>\r\n", /* command help text */
		cli_handler_gyro,		 /* The function to run. */
		1						 /* The user can enter any number of parameters */
};
/******************************************************************************/
/* Static Function Definitions                                                */
/******************************************************************************/
int16_t read_gyro_value(){
	cy_rslt_t rslt;
	uint8_t write_buffer[1];
	uint8_t read_buffer[1];
	int16_t toReturn;

	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);
	xSemaphoreTake(Semaphore_Gyro, portMAX_DELAY);
	
	// write Z High Reg into register_val
	write_buffer[0] = GYRO_ZOUT_H;
	rslt = cyhal_i2c_master_write(
		&i2c_master_obj,
		IAM20380_SUBORDINATE_ADDR,
		write_buffer,
		1,
		0,
		false);
	if (rslt != CY_RSLT_SUCCESS)
	{
		task_print_error("Error writing Z High reg address to the Gyroscope");
	}
	else
	{
		rslt = cyhal_i2c_master_read(
			&i2c_master_obj,		   
			IAM20380_SUBORDINATE_ADDR, 
			read_buffer,			   
			1,						   
			0,						   
			true);					   

		if (rslt != CY_RSLT_SUCCESS)
		{
			task_print_error("Error Reading from the Gyro Z High Register");
			read_buffer[0] = 0xFF;
		}
		gyro_z_value = read_buffer[0] << 8;
		cyhal_system_delay_us(20);

		// read Z Low Reg into register_val
		write_buffer[0] = GYRO_ZOUT_L;
		rslt = cyhal_i2c_master_write(
			&i2c_master_obj,
			IAM20380_SUBORDINATE_ADDR,
			write_buffer,
			1,
			0,
			false);
		if (rslt != CY_RSLT_SUCCESS)
		{
			task_print_error("Error writing Z Low reg address to the Gyroscope");
		}
		else
		{
			rslt = cyhal_i2c_master_read(
				&i2c_master_obj,		   
				IAM20380_SUBORDINATE_ADDR, 
				read_buffer,			   
				1,						   
				0,						   
				true);					   

			if (rslt != CY_RSLT_SUCCESS)
			{
				task_print_error("Error Reading from the Gyro Z Low Register");
				read_buffer[0] = 0xFF;
			}
			gyro_z_value |= read_buffer[0];
		}
	}
	toReturn = gyro_z_value;
	xSemaphoreGive(Semaphore_Gyro);
	xSemaphoreGive(Semaphore_I2C);
	return toReturn;
}

static void timer_callback_gyro(TimerHandle_t xTimer)
{ 
	read_gyro_value();
	task_print("z value: %ld\n", gyro_z_value);
	return;
}

/******************************************************************************/
/* Public Function Definitions                                                */
/******************************************************************************/

/**
 * @brief
 * Task used to monitor the reception of command packets sent the gyroscope
 * @param param
 * Unused
 */
void task_gyro(void *param)
{
	gyro_message_type_t packet;

	// do setup of gyroscope
	cy_rslt_t rslt;
	uint8_t write_buffer[2];
	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);

	// set PWR_MGMT_1 to 0x81
	write_buffer[0] = PWR_MGMT_1;
	write_buffer[1] = 0x81;
	rslt = cyhal_i2c_master_write(
		&i2c_master_obj,
		IAM20380_SUBORDINATE_ADDR, // I2C Address
		write_buffer,			   // Array of data to write
		2,						   // Number of bytes to write
		0,						   // Block until completed
		true);					   // Do NOT generate Stop Condition

	if (rslt != CY_RSLT_SUCCESS)
	{
		task_print_error("Error writing pwr_mgmt to the Gyroscope");
	}
	cyhal_system_delay_us(20);
	// set SMPLRT_DIV to 0x00(?) TODO verfiy value
	write_buffer[0] = SMPLRT_DIV;
	write_buffer[1] = 0x00;
	rslt = cyhal_i2c_master_write(
		&i2c_master_obj,
		IAM20380_SUBORDINATE_ADDR, // I2C Address
		write_buffer,			   // Array of data to write
		2,						   // Number of bytes to write
		0,						   // Block until completed
		true);					   // Do NOT generate Stop Condition

	if (rslt != CY_RSLT_SUCCESS)
	{
		task_print_error("Error writing smplrt to the Gyroscope");
	}
	cyhal_system_delay_us(20);

	// set GYRO_CONFIG to 0x10(?) TODO verify value
	write_buffer[0] = GYRO_CONFIG;
	write_buffer[1] = 0x10;
	rslt = cyhal_i2c_master_write(
		&i2c_master_obj,
		IAM20380_SUBORDINATE_ADDR, // I2C Address
		write_buffer,			   // Array of data to write
		2,						   // Number of bytes to write
		0,						   // Block until completed
		true);					   // Do NOT generate Stop Condition

	if (rslt != CY_RSLT_SUCCESS)
	{
		task_print_error("Error writing gyro_config to the Gyroscope");
	}
	cyhal_system_delay_us(20);

	// set CONFIG to 0x00(?) TODO verify value
	write_buffer[0] = CONFIG_REG;
	write_buffer[1] = 0x00;
	rslt = cyhal_i2c_master_write(
		&i2c_master_obj,
		IAM20380_SUBORDINATE_ADDR, // I2C Address
		write_buffer,			   // Array of data to write
		2,						   // Number of bytes to write
		0,						   // Block until completed
		true);					   // generate Stop Condition

	if (rslt != CY_RSLT_SUCCESS)
	{
		task_print_error("Error writing config address to the Gyroscope");
	}
	xSemaphoreGive(Semaphore_I2C);
	// loop for queue to toggle timer on and off
	while (1)
	{
		/* Wait for a message */
		xQueueReceive(q_gyro, &packet, portMAX_DELAY);
		// toggle the timer off and on, nothing else(?)
		if (packet == GYRO_ON)
		{
			xTimerStart(gyro_timer, portMAX_DELAY);
		}
		else if (packet == GYRO_OFF)
		{
			xTimerStop(gyro_timer, portMAX_DELAY);
		}
	}
}

static BaseType_t cli_handler_gyro(
	char *pcWriteBuffer,
	size_t xWriteBufferLen,
	const char *pcCommandString)
{
	BaseType_t xParameterStringLength, xReturn;
	gyro_message_type_t gyro_type;
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
		gyro_type = GYRO_ON;
	}
	else if (strcmp(pcWriteBuffer, "off") == 0)
	{
		gyro_type = GYRO_OFF;
	}
	else
	{
		/* Return a string indicating an invalid parameter. */
		memset(pcWriteBuffer, 0x00, xWriteBufferLen);
		sprintf(pcWriteBuffer, "Error... invalid parameter\n\r");
		xReturn = pdFALSE;
		return xReturn;
	}

	//  Send the message to the gyro task
	xQueueSendToBack(q_gyro, &gyro_type, portMAX_DELAY);

	/* Nothing to return, so zero out the pcWriteBuffer. */
	memset(pcWriteBuffer, 0, xWriteBufferLen);

	/* Indicate that the command has completed */
	xReturn = pdFALSE;

	return xReturn;
}

void task_gyro_init(void)
{
	//initialise Gyro semaphore
	Semaphore_Gyro = xSemaphoreCreateBinary();
	xSemaphoreGive(Semaphore_Gyro);
	/* Create the Queue used to control reading of the gyroscope registers*/
	q_gyro = xQueueCreate(1, sizeof(gyro_message_type_t));

	/* Register the CLI command */
	FreeRTOS_CLIRegisterCommand(&cmd_gyro);

	gyro_timer = xTimerCreate(
		"Gyro Timer",
		pdMS_TO_TICKS(100),
		true,
		0,
		timer_callback_gyro);
	/* Create the task that will control the gyroscope reading */
	xTaskCreate(
		task_gyro,
		"Task_Gyro",
		configMINIMAL_STACK_SIZE,
		NULL,
		configMAX_PRIORITIES - 6,
		NULL);
}
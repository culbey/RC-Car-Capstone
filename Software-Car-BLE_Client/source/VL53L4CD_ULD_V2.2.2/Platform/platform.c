/**
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */


#include "platform.h"
#include "main.h"
#include "i2c.h"


uint8_t VL53L4CD_RdDWord(Dev_inst_t dev, uint16_t RegisterAdress, uint32_t *value)
{
	uint8_t status = 255;
	cy_rslt_t rslt;
	uint8_t write_buffer[2];
	uint8_t dev_addr = 0x29;
	uint8_t read_buffer[4];
	write_buffer[0] = (RegisterAdress & 0xFF00) >> 8;
	write_buffer[1] = (RegisterAdress & 0xFF);
	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);
	rslt = cyhal_i2c_master_write(
		&i2c_master_obj,
		dev_addr,
		write_buffer,
		2,
		0,
		false); 
	if (rslt != CY_RSLT_SUCCESS) {
		task_print_error("Error writing to prox_sensor with rslt: %d\n", rslt);
	}
	rslt = cyhal_i2c_master_read(
		&i2c_master_obj,
		dev_addr,
		read_buffer,
		4,
		0,
		true);
	if (rslt != CY_RSLT_SUCCESS) {
		task_print_error("Error reading from prox_sensor with rslt: %d\n", rslt);
	}
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	xSemaphoreGive(Semaphore_I2C);
	*(value) = (read_buffer[3] << 24) | (read_buffer[2] << 16) | (read_buffer[1] << 8) | (read_buffer[0]);
	status = 0;
	return status;
}

uint8_t VL53L4CD_RdWord(Dev_inst_t dev, uint16_t RegisterAdress, uint16_t *value)
{
	uint8_t status = 255;
	cy_rslt_t rslt;
	uint8_t write_buffer[2];
	uint8_t dev_addr = 0x29;
	uint8_t read_buffer[2];
	write_buffer[0] = (RegisterAdress & 0xFF00) >> 8;
	write_buffer[1] = (RegisterAdress & 0xFF);
	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);
	rslt = cyhal_i2c_master_write(
		&i2c_master_obj,
		dev_addr,
		write_buffer,
		2,
		0,
		false); 
	if (rslt != CY_RSLT_SUCCESS) {
		task_print_error("Error writing to prox_sensor with rslt: %d\n", rslt);
	}
	rslt = cyhal_i2c_master_read(
		&i2c_master_obj,
		dev_addr,
		read_buffer,
		2,
		0,
		true);
	if (rslt != CY_RSLT_SUCCESS) {
		task_print_error("Error reading from prox_sensor with rslt: %d\n", rslt);
	}
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	xSemaphoreGive(Semaphore_I2C);
	*(value) = (read_buffer[1] << 8) | read_buffer[0];
	status = 0;
	return status;
}

uint8_t VL53L4CD_RdByte(Dev_inst_t dev, uint16_t RegisterAdress, uint8_t *value)
{
	uint8_t status = 255;	
	cy_rslt_t rslt;
	uint8_t write_buffer[2];
	uint8_t dev_addr = 0x29;
	uint8_t read_buffer[1];
	write_buffer[0] = (RegisterAdress & 0xFF00) >> 8;
	write_buffer[1] = (RegisterAdress & 0xFF);
	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);
	rslt = cyhal_i2c_master_write(
		&i2c_master_obj,
		dev_addr,
		write_buffer,
		2,
		0,
		false); 
	if (rslt != CY_RSLT_SUCCESS) {
		task_print_error("Error writing to prox_sensor with rslt: %d\n", rslt);
	}
	rslt = cyhal_i2c_master_read(
		&i2c_master_obj,
		dev_addr,
		read_buffer,
		1,
		0,
		true);
	if (rslt != CY_RSLT_SUCCESS) {
		task_print_error("Error reading from prox_sensor with rslt: %d\n", rslt);
	}
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	xSemaphoreGive(Semaphore_I2C);
	*(value) = read_buffer[0];
	if (rslt == CY_RSLT_SUCCESS) {
		status = 0;
	}
	return status;
}

uint8_t VL53L4CD_WrByte(Dev_inst_t dev, uint16_t RegisterAdress, uint8_t value)
{
	uint8_t status = 255;
	cy_rslt_t rslt;
	uint8_t write_buffer[3];
	uint8_t dev_addr = 0x29;
	write_buffer[0] = (RegisterAdress & 0xFF00) >> 8;
	write_buffer[1] = (RegisterAdress & 0xFF);
	write_buffer[2] = (value & 0xFF);
	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);
	rslt = cyhal_i2c_master_write(
		&i2c_master_obj,
		dev_addr,
		write_buffer,
		3,
		0,
		true);
	if (rslt != CY_RSLT_SUCCESS) {
		task_print_error("Error writing to prox_sensor with rslt: %d\n", rslt);
	}
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	xSemaphoreGive(Semaphore_I2C);
	status = 0;
	return status;
}

uint8_t VL53L4CD_WrWord(Dev_inst_t dev, uint16_t RegisterAdress, uint16_t value)
{	
	uint8_t status = 255;
	cy_rslt_t rslt;
	uint8_t write_buffer[4];
	uint8_t dev_addr = 0x29;
	write_buffer[0] = (RegisterAdress & 0xFF00) >> 8;
	write_buffer[1] = (RegisterAdress & 0xFF);
	write_buffer[2] = (value & 0xFF00) >> 8;
	write_buffer[3] = (value & 0xFF);
	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);
	rslt = cyhal_i2c_master_write(
		&i2c_master_obj,
		dev_addr,
		write_buffer,
		4,
		0,
		true);
	if (rslt != CY_RSLT_SUCCESS) {
		task_print_error("Error writing to prox_sensor with rslt: %d\n", rslt);
	}
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	xSemaphoreGive(Semaphore_I2C);
	status = 0;
	return status;
}

uint8_t VL53L4CD_WrDWord(Dev_inst_t dev, uint16_t RegisterAdress, uint32_t value)
{
	uint8_t status = 255;
	cy_rslt_t rslt;
	uint8_t write_buffer[6];
	uint8_t dev_addr = 0x29;
	write_buffer[0] = (RegisterAdress & 0xFF00) >> 8;
	write_buffer[1] = (RegisterAdress & 0xFF);
	write_buffer[2] = (value & 0xFF000000) >> 24;
	write_buffer[3] = (value & 0xFF0000) >> 16;
	write_buffer[4] = (value & 0xFF00) >> 8;
	write_buffer[5] = (value & 0xFF);
	xSemaphoreTake(Semaphore_I2C, portMAX_DELAY);
	rslt = cyhal_i2c_master_write(
		&i2c_master_obj,
		dev_addr,
		write_buffer,
		6,
		0,
		true);
	if (rslt != CY_RSLT_SUCCESS) {
		task_print_error("Error writing to prox_sensor with rslt: %d\n", rslt);
	}
	/* To be filled by customer. Return 0 if OK */
	/* Warning : For big endian platforms, fields 'RegisterAdress' and 'value' need to be swapped. */
	xSemaphoreGive(Semaphore_I2C);
	status = 0;
	return status;
}

uint8_t VL53L4CD_WaitMs(Dev_inst_t dev, uint32_t TimeMs)
{
	uint8_t status = 255;
	/* To be filled by customer */
	cy_rslt_t rslt;
	rslt = cyhal_system_delay_ms(TimeMs);
	if (rslt != CY_RSLT_SUCCESS) {
		task_print_error("Error while delaying system\n");
	}
	status = 0;
	return status;
}

/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mpsl_fem_twi_drv.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/i2c/i2c_nrfx_twim.h>

int32_t mpsl_fem_twi_drv_impl_xfer_read(void *p_instance, uint8_t slave_address,
			uint8_t internal_address, uint8_t *p_data, uint8_t data_length)
{
	const struct device *dev = (const struct device *)p_instance;

	return i2c_burst_read(dev, slave_address, internal_address, p_data, data_length);
}

int32_t mpsl_fem_twi_drv_impl_xfer_write(void *p_instance, uint8_t slave_address,
			uint8_t internal_address, const uint8_t *p_data, uint8_t data_length)
{
	const struct device *dev = (const struct device *)p_instance;

	return i2c_burst_write(dev, slave_address, internal_address, p_data, data_length);
}

int32_t mpsl_fem_twi_drv_impl_xfer_write_async(void * p_instance, uint8_t slave_address,
	const uint8_t * p_data, uint8_t data_length, mpsl_fem_twi_async_xfer_write_cb_t p_callback,
	void * p_context)
{
	const struct device *dev = (const struct device *)p_instance;

	struct i2c_msg msg = {
		.buf = (uint8_t*)p_data,
		.len = data_length,
		.flags = I2C_MSG_WRITE | I2C_MSG_STOP
	};

	return i2c_nrfx_twim_async_transfer_begin(dev, &msg, slave_address,
		(i2c_nrfx_twim_async_transfer_handler_t)p_callback, p_context);
}

uint32_t mpsl_fem_twi_drv_impl_xfer_write_async_time_get(void * p_instance, uint8_t data_length)
{
	(void)p_instance;

	static const uint32_t sw_overhead_safety_margin_time_us = 10;

	return sw_overhead_safety_margin_time_us + 
		(1U + (1U + 8U + 1U) + (1U + 8U + 1U) * data_length + 2U) * 10;
}

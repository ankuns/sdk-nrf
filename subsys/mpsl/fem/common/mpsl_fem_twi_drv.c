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

static mpsl_fem_twi_async_xfer_write_cb_t async_xfer_write_cb;
static uint8_t async_xfer_buf[2];

static void twim_async_transfer_handler(const struct device *dev, int res)
{
	void * p_instance = (void*)dev;

	async_xfer_write_cb(p_instance, res);
}

int32_t mpsl_fem_twi_drv_impl_xfer_write_async(void * p_instance, uint8_t slave_address,
			uint8_t internal_address, const uint8_t * p_data, uint8_t data_length,
			mpsl_fem_twi_async_xfer_write_cb_t p_callback)
{
	const struct device *dev = (const struct device *)p_instance;	

	async_xfer_buf[0] = internal_address;
	async_xfer_buf[1] = *p_data;

	struct i2c_msg msg = {
		.buf = async_xfer_buf,
		.len = 2,
		.flags = I2C_MSG_WRITE | I2C_MSG_STOP
	};

	async_xfer_write_cb = p_callback;

	return i2c_nrfx_twim_async_transfer_begin(dev, &msg, slave_address, twim_async_transfer_handler);
}

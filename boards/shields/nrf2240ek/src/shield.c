/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/i2c.h>

#define PMIC_REGADDR_VBUSIN_TASKUPDATEILIMSW 0x0200U

#define PMIC_REGVAL_VBUSIN_TASKUPDATEILIMSW_NOEFFECT     0x00U
#define PMIC_REGVAL_VBUSIN_TASKUPDATEILIMSW_SELVBUSILIM0 0x01U

#define PMIC_REGADDR_VBUSIN_VBUSINILIM0      0x0201U

#define PMIC_REGVAL_VBUSIN_VBUSINILIM0_500MA0 0U

/**
 * @brief Writes a register of a PMIC device through I2C.
 *
 * @param spec     I2C specification from the devicetree.
 * @param reg_addr Address of the register within PMIC device.
 * @param value    The value to write into the register.
 *
 * @return As underlying @ref i2c_write_dt function. @c 0 if successful.
 */
static inline int nrf2240ek_pmic_reg_write_dt(
		const struct i2c_dt_spec *spec,
		uint16_t reg_addr,
		uint8_t value)
{
	uint8_t tx_buf[3] = {
		(uint8_t)(reg_addr >> 8),
		(uint8_t)reg_addr,
		value
	};

	return i2c_write_dt(spec, tx_buf, 3);
}

static int nrf2240ek_pmic_init(void)
{
	int err;
	struct i2c_dt_spec bus = I2C_DT_SPEC_GET(DT_NODELABEL(nrf2240ek_pmic));

	err = nrf2240ek_pmic_reg_write_dt(&bus,
		PMIC_REGADDR_VBUSIN_TASKUPDATEILIMSW,
		PMIC_REGVAL_VBUSIN_TASKUPDATEILIMSW_SELVBUSILIM0);

	if (err != 0) {
		return err;
	}

	err = nrf2240ek_pmic_reg_write_dt(&bus,
		PMIC_REGADDR_VBUSIN_VBUSINILIM0,
		PMIC_REGVAL_VBUSIN_VBUSINILIM0_500MA0);

	if (err != 0) {
		return err;
	}

	return 0;
}

static int shield_init(void)
{
	return nrf2240ek_pmic_init();
}

SYS_INIT(shield_init, POST_KERNEL, CONFIG_SHIELD_NRF2240EK_PMIC_INIT_PRIORITY);

BUILD_ASSERT(CONFIG_MPSL_FEM_INIT_PRIORITY > CONFIG_SHIELD_NRF2240EK_PMIC_INIT_PRIORITY,
	"The initialization of nRF2240 Front-End Module must happen after initialization of the PMIC on the nRF2240EK shield.");

BUILD_ASSERT(CONFIG_SHIELD_NRF2240EK_PMIC_INIT_PRIORITY > CONFIG_I2C_INIT_PRIORITY,
	"The initialization of the PMIC on the nRF2240EK shield must happen after initialization of I2C.");

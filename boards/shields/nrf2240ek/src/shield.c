/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/sensor.h>

static int nrf2240ek_pmic_init(void)
{
	const struct device *dev = DEVICE_DT_GET(DT_NODELABEL(nrf2240ek_pmic_charger));

	const struct sensor_value currentLimit = { .val1 = 0, .val2 = 500000 };
	sensor_attr_set(dev, SENSOR_CHAN_CURRENT, SENSOR_ATTR_CONFIGURATION, &currentLimit);

	return 0;
}

static int shield_init(void)
{
	return nrf2240ek_pmic_init();
}

SYS_INIT(shield_init, POST_KERNEL, CONFIG_SHIELD_NRF2240EK_NPM1300_INIT_PRIORITY);

BUILD_ASSERT(CONFIG_MPSL_FEM_INIT_PRIORITY > CONFIG_SHIELD_NRF2240EK_NPM1300_INIT_PRIORITY,
	"The initialization of nRF2240 Front-End Module on the nRF2240EK shield must happen after initialization of nPM1300 on the shield");

BUILD_ASSERT(CONFIG_SHIELD_NRF2240EK_NPM1300_INIT_PRIORITY > CONFIG_SENSOR_INIT_PRIORITY,
	"The initialization of the nPM1300 on the nRF2240EK shield must happen after sensor initialization");

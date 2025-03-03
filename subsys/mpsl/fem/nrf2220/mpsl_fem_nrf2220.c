/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#if defined(CONFIG_MPSL_FEM_NRF2220)

#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <string.h>
#include <zephyr/sys/__assert.h>
#include <soc_nrf_common.h>

#include <mpsl_fem_config_nrf2220.h>
#include <nrfx_gpiote.h>
#include <mpsl_fem_utils.h>
#include <mpsl_fem_twi_drv.h>

#if IS_ENABLED(CONFIG_HAS_HW_NRF_PPI)
#include <nrfx_ppi.h>
#elif IS_ENABLED(CONFIG_HAS_HW_NRF_DPPIC)
#include <nrfx_dppi.h>
#endif

#include <soc_secure.h>

#if IS_ENABLED(CONFIG_PINCTRL)
#include <pinctrl_soc.h>
#endif

#if !defined(CONFIG_PINCTRL)
#error "The nRF2220 driver must be used with CONFIG_PINCTRL! Set CONFIG_PINCTRL=y"
#endif

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), twi_if)
#define MPSL_FEM_TWI_IF     DT_PHANDLE(DT_NODELABEL(nrf_radio_fem), twi_if)
#endif

#define FEM_OUTPUT_POWER_DBM     DT_PROP(DT_NODELABEL(nrf_radio_fem), output_power_dbm)

#define PIN_NUM(node_id, prop, idx)  NRF_GET_PIN(DT_PROP_BY_IDX(node_id, prop, idx)),
#define PIN_FUNC(node_id, prop, idx) NRF_GET_FUN(DT_PROP_BY_IDX(node_id, prop, idx)),

#define NRF2220_OUTPUT_POWER_DBM_MIN 7
#define NRF2220_OUTPUT_POWER_DBM_MAX 14

BUILD_ASSERT(
	(FEM_OUTPUT_POWER_DBM >= NRF2220_OUTPUT_POWER_DBM_MIN) &&
	(FEM_OUTPUT_POWER_DBM <= NRF2220_OUTPUT_POWER_DBM_MAX),
	"Value of output-power-dbm property out of range.");

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), twi_if)
#if DT_NODE_HAS_PROP(MPSL_FEM_TWI_IF, init_regs)
static void fem_nrf2220_twi_init_regs_configure(mpsl_fem_nrf2220_interface_config_t *cfg)
{
	const int32_t fem_twi_init_regs[] = DT_PROP(MPSL_FEM_TWI_IF, init_regs);

	for (uint32_t i = 0; i < cfg->twi_regs_init_map.nb_regs; i++) {
		cfg->twi_regs_init_map.p_regs[i].addr = (uint8_t)fem_twi_init_regs[2 * i];
		cfg->twi_regs_init_map.p_regs[i].val = (uint8_t)fem_twi_init_regs[2 * i + 1];
	}
}
#endif /* DT_NODE_HAS_PROP(MPSL_FEM_TWI_IF, init_regs) */

static void fem_nrf2220_twi_configure(mpsl_fem_nrf2220_interface_config_t *cfg)
{
	cfg->twi_if = MPSL_FEM_TWI_DRV_IF_INITIALIZER(MPSL_FEM_TWI_IF);
}
#endif /* DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), twi_if) */

/* Required for testing only. */
static uint8_t cs_gpiote_channel;
static uint8_t md_gpiote_channel;

uint8_t nrf2220_cs_gpiote_channel_get(void)
{
	return cs_gpiote_channel;
}

uint8_t nrf2220_md_gpiote_channel_get(void)
{
	return md_gpiote_channel;
}

static int fem_nrf2220_configure(void)
{
	int err;

	const nrfx_gpiote_t cs_gpiote = NRFX_GPIOTE_INSTANCE(
		NRF_DT_GPIOTE_INST(DT_NODELABEL(nrf_radio_fem), cs_gpios));
	if (nrfx_gpiote_channel_alloc(&cs_gpiote, &cs_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	const nrfx_gpiote_t md_gpiote = NRFX_GPIOTE_INSTANCE(
		NRF_DT_GPIOTE_INST(DT_NODELABEL(nrf_radio_fem), md_gpios));
	if (nrfx_gpiote_channel_alloc(&md_gpiote, &md_gpiote_channel) != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	mpsl_fem_nrf2220_interface_config_t cfg = {
		.fem_config = {
			.output_power_dbm = FEM_OUTPUT_POWER_DBM,
			.bypass_gain_db = DT_PROP(DT_NODELABEL(nrf_radio_fem), bypass_gain_db),
		},
		.cs_pin_config = {
			.gpio_pin      = {
				.p_port   = MPSL_FEM_GPIO_PORT_REG(cs_gpios),
				.port_no  = MPSL_FEM_GPIO_PORT_NO(cs_gpios),
				.port_pin = MPSL_FEM_GPIO_PIN_NO(cs_gpios),
			},
			.enable        = true,
			.active_high   = MPSL_FEM_GPIO_POLARITY_GET(cs_gpios),
			.gpiote_ch_id  = cs_gpiote_channel,
#if defined(NRF54L_SERIES)
			.p_gpiote = cs_gpiote.p_reg,
#endif
		},
		.md_pin_config = {
			.gpio_pin      = {
				.p_port   = MPSL_FEM_GPIO_PORT_REG(md_gpios),
				.port_no  = MPSL_FEM_GPIO_PORT_NO(md_gpios),
				.port_pin = MPSL_FEM_GPIO_PIN_NO(md_gpios),
			},
			.enable        = true,
			.active_high   = MPSL_FEM_GPIO_POLARITY_GET(md_gpios),
			.gpiote_ch_id  = md_gpiote_channel,
#if defined(NRF54L_SERIES)
			.p_gpiote = md_gpiote.p_reg,
#endif
		}
	};

#if DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), twi_if)
	fem_nrf2220_twi_configure(&cfg);
#endif

#if DT_NODE_HAS_PROP(MPSL_FEM_TWI_IF, init_regs)
	const int32_t fem_twi_init_regs[] = DT_PROP(MPSL_FEM_TWI_IF, init_regs);
	const uint8_t nb_regs = ARRAY_SIZE(fem_twi_init_regs) / 2;

	BUILD_ASSERT(ARRAY_SIZE(fem_twi_init_regs) % 2 == 0,
		"The number of specified nRF2220 TWI initialization registers to write differs from"
		" the number of values specified for those registers.");

	mpsl_fem_twi_reg_val_t init_regs[nb_regs];

	cfg.twi_regs_init_map.p_regs = init_regs;
	cfg.twi_regs_init_map.nb_regs = nb_regs;

	fem_nrf2220_twi_init_regs_configure(&cfg);
#endif /* DT_NODE_HAS_PROP(MPSL_FEM_TWI_IF, init_regs) */

	IF_ENABLED(CONFIG_HAS_HW_NRF_PPI,
		   (err = mpsl_fem_utils_ppi_channel_alloc(cfg.ppi_channels,
						ARRAY_SIZE(cfg.ppi_channels));));
	IF_ENABLED(CONFIG_HAS_HW_NRF_DPPIC,
		   (err = mpsl_fem_utils_ppi_channel_alloc(cfg.dppi_channels,
						ARRAY_SIZE(cfg.dppi_channels));));
	if (err) {
		return err;
	}

	err = mpsl_fem_utils_gpiote_pin_init(&cfg.cs_pin_config);
	if (err) {
		return err;
	}

	err = mpsl_fem_utils_gpiote_pin_init(&cfg.md_pin_config);
	if (err) {
		return err;
	}

	err = mpsl_fem_nrf2220_interface_config_set(&cfg);

	return err;
}

static int mpsl_fem_init(void)
{
	mpsl_fem_device_config_254_apply_set(IS_ENABLED(CONFIG_MPSL_FEM_DEVICE_CONFIG_254));

	return fem_nrf2220_configure();
}

#if defined(CONFIG_I2C) && \
	DT_NODE_HAS_PROP(DT_NODELABEL(nrf_radio_fem), twi_if)
BUILD_ASSERT(CONFIG_MPSL_FEM_INIT_PRIORITY > CONFIG_I2C_INIT_PRIORITY,
	"The initialization of nRF2220 Front-End Module must happen after initialization of I2C");
#endif

SYS_INIT(mpsl_fem_init, POST_KERNEL, CONFIG_MPSL_FEM_INIT_PRIORITY);

#if defined(CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_COMPENSATION)

#include <protocol/mpsl_fem_nrf2220_protocol_api.h>
#include <zephyr/drivers/i2c/i2c_nrfx_twim.h>

static K_SEM_DEFINE(fem_temperature_sem, 0, 1);
static volatile int8_t fem_temperature;
static K_SEM_DEFINE(fem_temperature_updated_sem, 0, 1);

void fem_temperature_changed(int8_t temperature)
{
	fem_temperature = temperature;
	k_sem_give(&fem_temperature_sem);
}

static void fem_temperature_update_cb(int32_t res)
{
	(void)res;

	k_sem_give(&fem_temperature_updated_sem);
}

static void fem_temperature_compensation_thread(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	static const struct device * i2c_bus_dev = DEVICE_DT_GET(DT_BUS(MPSL_FEM_TWI_IF));

	while (true) {
		// k_sem_take(&fem_temperature_sem, K_FOREVER);
		k_msleep(100);

		printk("mpsl_fem_nrf2220_temperature_changed(%d)", (int)fem_temperature);
		if (mpsl_fem_nrf2220_temperature_changed(fem_temperature)) {
			printk("   need update\n");
			/* Let's have "taken" semaphore that will inform us that the operation on the
			 * nrf2220 is finished.
			 */
			k_sem_take(&fem_temperature_updated_sem, K_NO_WAIT);

			printk("mpsl_fem_nrf2220_temperature_update_request\n");
			

			(void)i2c_nrfx_twim_exclusive_access_acquire(i2c_bus_dev, K_FOREVER);

			mpsl_fem_nrf2220_temperature_changed_update_request(fem_temperature_update_cb);
		
			/* Let's wait until the operation is finished */
			k_sem_take(&fem_temperature_updated_sem, K_FOREVER);
			
			i2c_nrfx_twim_exclusive_access_release(i2c_bus_dev);


			printk("   updated\n");

		}
		else {
			printk("   nothing to do\n");
		}

		// Simulate temperature changes
		if (fem_temperature < 120) {
			fem_temperature += 5;
		}
		else {
			fem_temperature = -50;
		}
	}
}

#define FEM_TEMPERATURE_COMPENSATION_THREAD_STACK_SIZE 1024

K_THREAD_DEFINE(fem_temp_comp, FEM_TEMPERATURE_COMPENSATION_THREAD_STACK_SIZE,
		fem_temperature_compensation_thread,
		NULL, NULL, NULL,
		K_LOWEST_APPLICATION_THREAD_PRIO,
		0, 0);

#endif /* CONFIG_MPSL_FEM_NRF2220_TEMPERATURE_COMPENSATION */

#endif /* defined(CONFIG_MPSL_FEM_NRF2220) */

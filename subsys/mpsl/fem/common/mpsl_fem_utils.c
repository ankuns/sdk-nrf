/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#include <mpsl_fem_utils.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/__assert.h>
#include <hal/nrf_gpio.h>
#if IS_ENABLED(CONFIG_HAS_HW_NRF_PPI)
#include <nrfx_ppi.h>
#elif defined(CONFIG_SOC_SERIES_NRF54LX)
#include <helpers/nrfx_flag32_allocator.h>
#include <soc/interconnect/dppic_ppib/nrfx_interconnect_dppic_ppib.h>
#elif IS_ENABLED(CONFIG_HAS_HW_NRF_DPPIC)
#include <nrfx_dppi.h>
#endif

int mpsl_fem_utils_ppi_channel_alloc(uint8_t *ppi_channels, size_t size)
{
	nrfx_err_t err = NRFX_ERROR_NOT_SUPPORTED;

	for (int i = 0; i < size; i++) {
		IF_ENABLED(CONFIG_HAS_HW_NRF_PPI,
			(err = nrfx_ppi_channel_alloc(&ppi_channels[i]);));
#if defined(CONFIG_SOC_SERIES_NRF54LX)
		/* Allocate DPPI channels within the Radio Power Domain */
		err = nrfx_flag32_alloc(
			&(nrfx_interconnect_dppic_get(NRF_APB_INDEX_RADIO)->channels_mask),
			&ppi_channels[i]);
#else
		IF_ENABLED(CONFIG_HAS_HW_NRF_DPPIC,
			(err = nrfx_dppi_channel_alloc(&ppi_channels[i]);));
#endif
		if (err != NRFX_SUCCESS) {
			return -ENOMEM;
		}
	}

	return 0;
}

void mpsl_fem_extended_pin_to_mpsl_fem_pin(uint32_t pin_num, mpsl_fem_pin_t *p_fem_pin)
{
	// pin_num is saved, because nrf_gpio_pin_port_number_extract overwrites it and
	// its original value is needed for nrf_gpio_pin_port_decode
	uint32_t pin_num_copy = pin_num;

	p_fem_pin->port_no  = nrf_gpio_pin_port_number_extract(&pin_num_copy);
	p_fem_pin->p_port   = nrf_gpio_pin_port_decode(&pin_num);

	p_fem_pin->port_pin = pin_num;
}

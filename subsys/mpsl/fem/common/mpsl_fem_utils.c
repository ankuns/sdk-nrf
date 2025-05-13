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
#elif IS_ENABLED(CONFIG_HAS_HW_NRF_DPPIC)
#include <nrfx_dppi.h>
#endif
#if defined(NRF54L_SERIES)
#include <hal/nrf_gpiote.h>
#include <nrfx_ppib.h>
#include <helpers/nrfx_gppi.h>
#endif

#if IS_ENABLED(CONFIG_HAS_HW_NRF_DPPIC)
static bool mpsl_fem_utils_nrfx_dppic_get(uint32_t periph_addr, nrfx_dppi_t *dppic)
{
#if defined(NRF53_SERIES)
	(void)periph_addr;
#if NRFX_DPPI0_ENABLED
	*dppic = (nrfx_dppi_t)NRFX_DPPI_INSTANCE(0);
	return true;
#else
	*dppic = (nrfx_dppi_t){0};
	return false;
#endif
#elif defined(NRF54L_SERIES)
	switch (nrf_address_bus_get(periph_addr, 0x4000)) {
#if NRFX_DPPI00_ENABLED
	case NRF_APB_INDEX_MCU:
		*dppic = (nrfx_dppi_t)NRFX_DPPI_INSTANCE(00);
		break;
#endif
#if NRFX_DPPI10_ENABLED
	case NRF_APB_INDEX_RADIO:
		*dppic = (nrfx_dppi_t)NRFX_DPPI_INSTANCE(10);
		break;
#endif
#if NRFX_DPPI20_ENABLED
	case NRF_APB_INDEX_PERI:
		*dppic = (nrfx_dppi_t)NRFX_DPPI_INSTANCE(20);
		break;
#endif
#if NRFX_DPPI30_ENABLED
	case NRF_APB_INDEX_LP:
		*dppic = (nrfx_dppi_t)NRFX_DPPI_INSTANCE(30);
		break;
#endif
	default:
		*dppic = (nrfx_dppi_t){0};
		return false;
	}

	return true;
#else
#error Unsupported SoC
#endif
}

int mpsl_fem_utils_dppi_channel_for_periph_alloc(uint32_t periph_addr, NRF_DPPIC_Type * *dppic,
						 uint8_t *dppi_channels, size_t size)
{
	nrfx_dppi_t nrfx_dppi = {0};

	if (!mpsl_fem_utils_nrfx_dppic_get(periph_addr, &nrfx_dppi)) {
		return -ENXIO;
	}

	if (dppic != NULL) {
		*dppic = nrfx_dppi.p_reg;
	}

	if (size != 0) {
		__ASSERT_NO_MSG(dppi_channels != NULL);

		for (int i = 0; i < size; i++) {
			if (NRFX_SUCCESS != nrfx_dppi_channel_alloc(&nrfx_dppi,
				&dppi_channels[i])) {
				return -ENOMEM;
			}
		}
	}

	return 0;
}
#endif /* IS_ENABLED(CONFIG_HAS_HW_NRF_DPPIC) */

int mpsl_fem_utils_ppi_channel_alloc(uint8_t *ppi_channels, size_t size)
{
#if IS_ENABLED(CONFIG_HAS_HW_NRF_PPI)
	for (int i = 0; i < size; i++) {
		if (NRFX_SUCCESS != nrfx_ppi_channel_alloc(&ppi_channels[i])) {
			return -ENOMEM;
		}
	}
	return 0;
#elif IS_ENABLED(CONFIG_HAS_HW_NRF_DPPIC)
	return mpsl_fem_utils_dppi_channel_for_periph_alloc((uintptr_t)NRF_RADIO, NULL,
							    ppi_channels, size);
#else
#error Unsupported SoC
#endif
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

#if defined(NRF54L_SERIES)
int mpsl_fem_utils_ppib11_to_peripheral_task_init(uint32_t tep, uint8_t *ppib_ch)
{
	nrfx_err_t err;
	uint8_t gppi_ch = 0;
	nrfx_ppib_interconnect_t ppib11_21 = NRFX_PPIB_INTERCONNECT_INSTANCE(11, 21);

	err = nrfx_ppib_channel_alloc(&ppib11_21, ppib_ch);
	if (err != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	err = nrfx_gppi_channel_alloc(&gppi_ch);
	if (err != NRFX_SUCCESS) {
		return -ENOMEM;
	}

	nrfx_gppi_channel_endpoints_setup(gppi_ch,
		nrfx_ppib_receive_event_address_get(&ppib11_21.right, *ppib_ch),
		tep);

	nrfx_gppi_channels_enable(1U << gppi_ch);

	return 0;
}
#endif

int mpsl_fem_utils_gpiote_pin_init(mpsl_fem_gpiote_pin_config_t *gpiote_pin)
{
#if defined(NRF54L_SERIES)
	int r;

	r = mpsl_fem_utils_ppib11_to_peripheral_task_init(
		nrf_gpiote_task_address_get(gpiote_pin->p_gpiote,
			nrf_gpiote_clr_task_get(gpiote_pin->gpiote_ch_id)),
		&gpiote_pin->ppib_channels[0]);

	if (r != 0) {
		return r;
	}

	r = mpsl_fem_utils_ppib11_to_peripheral_task_init(
		nrf_gpiote_task_address_get(gpiote_pin->p_gpiote,
			nrf_gpiote_set_task_get(gpiote_pin->gpiote_ch_id)),
		&gpiote_pin->ppib_channels[1]);

	return r;
#else
	return 0;
#endif
}

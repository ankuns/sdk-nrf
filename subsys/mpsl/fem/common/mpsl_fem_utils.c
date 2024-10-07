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
#include <helpers/nrfx_flag32_allocator.h>
#include <hal/nrf_ppib.h>
#include <hal/nrf_gpiote.h>
#include <nrf_802154_peripherals.h>

#define DPPIC10_CHANNELS_MASK_USED \
	(NRFX_DPPI_CHANNELS_USED)

#define DPPIC20_CHANNELS_MASK_USED \
	((1U << NRF_802154_DPPI_RADIO_HW_TRIGGER) \
	 (NRFX_BIT_MASK(4)))

#define PPIB11_21_PPIB_CHANNELS_MASK_USED_BY_BT_CTRL \
	(NRFX_BIT_MASK(4))

/* Note: a ppib11-ppib21 channel is used indirectly as a result of
 * nrf_802154_platform_sl_lptimer_hw_task_local_domain_connections_setup
 * on nRF54L.
 * The function nrf_802154_platform_sl_lptimer_hw_task_local_domain_connections_setup
 * could switch to common PPIB channel allocator.
 */
#define PPIB11_21_PPIB_CHANNELS_MASK_USED_BY_154 \
	(1U << NRF_802154_DPPI_RADIO_HW_TRIGGER)

#define PPIB11_21_PPIB_CHANNELS_MASK_USED               \
	(PPIB11_21_PPIB_CHANNELS_MASK_USED_BY_BT_CTRL | \
	PPIB11_21_PPIB_CHANNELS_MASK_USED_BY_154)

static nrfx_atomic_t dppic10_dppi_channels_mask =
	NRFX_BIT_MASK(24) & (~DPPIC10_CHANNELS_MASK_USED);

static nrfx_err_t dppic10_dppi_channel_alloc(uint8_t * p_channel)
{
	return nrfx_flag32_alloc(&dppic10_dppi_channels_mask, p_channel);
}

static nrfx_atomic_t dppic20_dppi_channels_mask =
	NRFX_BIT_MASK(16) & ~(1U << NRF_802154_DPPI_RADIO_HW_TRIGGER);

static nrfx_err_t dppic20_dppi_channel_alloc(uint8_t * p_channel)
{
	return nrfx_flag32_alloc(&dppic20_dppi_channels_mask, p_channel);
}

static nrfx_atomic_t ppib11_21_ppib_channels_mask = \
	(NRFX_BIT_MASK(16) & (~PPIB11_21_PPIB_CHANNELS_MASK_USED));

/* Allocates a PPIB channel connecting PPIB11 and PPIB21 */
static nrfx_err_t ppib11_21_ppib_channel_alloc(uint8_t * p_channel)
{
	return nrfx_flag32_alloc(&ppib11_21_ppib_channels_mask, p_channel);
}
#endif /* defined(NRF54L_SERIES) */

int mpsl_fem_utils_ppi_channel_alloc(uint8_t *ppi_channels, size_t size)
{
	nrfx_err_t err = NRFX_ERROR_NOT_SUPPORTED;

	for (int i = 0; i < size; i++) {
#if defined(NRF54L_SERIES)
		err = dppic10_dppi_channel_alloc(&ppi_channels[i]);
#else
		IF_ENABLED(CONFIG_HAS_HW_NRF_PPI,
			(err = nrfx_ppi_channel_alloc(&ppi_channels[i]);));
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

int mpsl_fem_utils_gpiote_pin_init(mpsl_fem_gpiote_pin_config_t *gpiote_pin)
{
#if defined(NRF54L_SERIES)
	nrfx_err_t err;

	gpiote_pin->p_gpiote = NRF_GPIOTE20; // TODO: select proper instance based on port.

	uint8_t ppib_ch = 0;
	uint8_t dppi_ch = 0;

	err = ppib11_21_ppib_channel_alloc(&ppib_ch);
	if (err != NRFX_SUCCESS)
	{
		return -ENOMEM;
	}

	err = dppic20_dppi_channel_alloc(&dppi_ch);
	if (err != NRFX_SUCCESS)
	{
		return -ENOMEM;
	}

	nrf_ppib_publish_set(NRF_PPIB21, nrf_ppib_receive_event_get(ppib_ch), dppi_ch);
	nrf_gpiote_subscribe_set(gpiote_pin->p_gpiote, nrf_gpiote_clr_task_get(gpiote_pin->gpiote_ch_id), dppi_ch);
	nrf_dppi_channels_enable(NRF_DPPIC20, 1UL << dppi_ch);
	gpiote_pin->ppib_channels[0] = ppib_ch;

	err = ppib11_21_ppib_channel_alloc(&ppib_ch);
	if (err != NRFX_SUCCESS)
	{
		return -ENOMEM;
	}

	err = dppic20_dppi_channel_alloc(&dppi_ch);
	if (err != NRFX_SUCCESS)
	{
		return -ENOMEM;
	}

	nrf_ppib_publish_set(NRF_PPIB21, nrf_ppib_receive_event_get(ppib_ch), dppi_ch);
	nrf_gpiote_subscribe_set(gpiote_pin->p_gpiote, nrf_gpiote_set_task_get(gpiote_pin->gpiote_ch_id), dppi_ch);
	nrf_dppi_channels_enable(NRF_DPPIC20, 1UL << dppi_ch);
	gpiote_pin->ppib_channels[1] = ppib_ch;
#endif
	return 0;
}
/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef MPSL_FEM_UTILS_H__
#define MPSL_FEM_UTILS_H__

#include <zephyr/sys/util.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <mpsl_fem_config_common.h>
#include <nrf.h>
#if IS_ENABLED(CONFIG_HAS_HW_NRF_DPPIC)
#include "hal/nrf_dppi.h"
#endif

#define MPSL_FEM_GPIO_POLARITY_GET(dt_property) \
	((GPIO_ACTIVE_LOW & \
	  DT_GPIO_FLAGS(DT_NODELABEL(nrf_radio_fem), dt_property)) \
	 ? false : true)

#define MPSL_FEM_GPIO_PORT(pin)     DT_GPIO_CTLR(DT_NODELABEL(nrf_radio_fem), pin)
#define MPSL_FEM_GPIO_PORT_REG(pin) ((NRF_GPIO_Type *)DT_REG_ADDR(MPSL_FEM_GPIO_PORT(pin)))
#define MPSL_FEM_GPIO_PORT_NO(pin)  DT_PROP(MPSL_FEM_GPIO_PORT(pin), port)
#define MPSL_FEM_GPIO_PIN_NO(pin)   DT_GPIO_PIN(DT_NODELABEL(nrf_radio_fem), pin)

#define MPSL_FEM_GPIO_INVALID_PIN        0xFFU
#define MPSL_FEM_GPIOTE_INVALID_CHANNEL  0xFFU

#define MPSL_FEM_DISABLED_GPIOTE_PIN_CONFIG_INIT	\
	.gpio_pin      = {				\
		.port_pin = MPSL_FEM_GPIO_INVALID_PIN,	\
	},						\
	.enable        = false,				\
	.active_high   = true,				\
	.gpiote_ch_id  = MPSL_FEM_GPIOTE_INVALID_CHANNEL

#define MPSL_FEM_DISABLED_GPIO_CONFIG_INIT		\
	.gpio_pin      = {				\
		.port_pin = MPSL_FEM_GPIO_INVALID_PIN,	\
	},						\
	.enable        = false,				\
	.active_high   = true,				\

#if defined(CONFIG_HAS_HW_NRF_DPPIC)
/** @brief Allocates free EGU instance.
 *
 * @param[out]  egu_instance_no  Number of allocated EGU instance.
 *
 * @return  0 in case of success, appropriate error code otherwise.
 */
static inline int mpsl_fem_utils_egu_instance_alloc(uint8_t *egu_instance_no)
{
	/* Always return EGU0. */
	*egu_instance_no = 0;

	return 0;
}

/** @brief Allocates free EGU channels and stores them in @p egu_channels.
 *
 * @param[out]  egu_channels     Array of allocated EGU channels.
 * @param[in]   size             Number of channels to allocate.
 * @param[in]   egu_instance_no  Number of EGU instance to allocate channels from.
 *
 * @return  0 in case of success, appropriate error code otherwise.
 */
static inline int mpsl_fem_utils_egu_channel_alloc(
	uint8_t *egu_channels, size_t size, uint8_t egu_instance_no)
{
	(void)egu_instance_no;

	/* The 802.15.4 radio driver is the only user of EGU peripheral on nRF5340 network core
	 * and it uses channels: 0, 1, 2, 3, 4, 15. Therefore starting from channel 5, a consecutive
	 * block of at most 10 channels can be allocated.
	 */
	if (size > 10U) {
		return -ENOMEM;
	}

	uint8_t starting_channel = 5U;

	for (int i = 0; i < size; i++) {
		egu_channels[i] = starting_channel + i;
	}

	return 0;
}

/** @brief Selects DPPIC controller and allocates DPPI channels to be used with given peripheral.
 *
 * For SoCs equipped with one DPPIC controller for a core like nRF5340
 * the only existing DPPIC controller is selected.
 *
 * For SoCs equipped with multiple DPPIC controllers for a core like nRF54L Series
 * this function selects a DPPIC controller appropriate for given peripheral so that
 * the peripheral can directly subscribe or publish to DPPI channels of the selected DPPIC
 * controller.
 *
 * The allocation of a DPPI channels occurs for the selected DPPIC controller.
 *
 *  @param[in]  periph_addr   Address of a peripheral to be used with a DPPIC controller.
 *  @param[out] dppic         DPPIC controller selected for given peripheral.
 *  @param[out] dppi_channels Array of allocated DPPI channels on the @p dppic
 *  @param[in]  size          Number of DPPI channels to allocate.
 *
 * @retval 0       Success.
 * @retval -ENXIO  An appropriate DPPIC controller for given @p periph_addr not found.
 * @retval -ENOMEM Not enough free DPPI channels on the selected DPPIC controller.
 */
int mpsl_fem_utils_dppi_channel_for_periph_alloc(uint32_t periph_addr, NRF_DPPIC_Type * *dppic,
						 uint8_t *dppi_channels, size_t size);

#endif /* defined(CONFIG_HAS_HW_NRF_DPPIC) */

/** @brief Allocates free (D)PPI channels and stores them in @p ppi_channels.
 *
 * For SoCs with multiple DPPIC controllers this function allocates DPPI channels
 * on the DPPIC controller that can be used in the domain of the RADIO peripheral.
 *
 * @param[out]  ppi_channels  Array of allocated (D)PPI channels.
 * @param[in]   size          Number of channels to allocate.
 *
 * @return  0 in case of success, appropriate error code otherwise.
 */
int mpsl_fem_utils_ppi_channel_alloc(uint8_t *ppi_channels, size_t size);

/** @brief Converts pin number extended with port number to an mpsl_fem_pin_t structure.
 *
 * @param[in]     pin_num    Pin number extended with port number.
 * @param[inout]  p_fem_pin  Pointer to be filled with pin represented as an mpsl_fem_pin_t struct.
 */
void mpsl_fem_extended_pin_to_mpsl_fem_pin(uint32_t pin_num, mpsl_fem_pin_t *p_fem_pin);

#if defined(NRF54L_SERIES)
int mpsl_fem_utils_ppib11_to_peripheral_task_init(uint32_t tep, uint8_t *ppib_ch);
#endif

/** @brief Initializes the gpiote pin according to the configuration.
 *
 * @param[inout] gpiote_pin Configuration of gpiote pin.
 *
 * @return 0 in case of success, appropriate error code otherwise.
 */
int mpsl_fem_utils_gpiote_pin_init(mpsl_fem_gpiote_pin_config_t *gpiote_pin);


#endif /* MPSL_FEM_UTILS_H__ */

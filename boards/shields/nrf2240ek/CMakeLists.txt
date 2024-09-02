#
# Copyright (c) 2024 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
#

if(CONFIG_SHIELD_NRF2240EK)

zephyr_library()
zephyr_library_sources_ifdef(CONFIG_SHIELD_NRF2240EK_PMIC_CURRENT_LIMIT_WORKAROUND src/shield.c)

endif()

#
# Copyright (c) 2020 Nordic Semiconductor ASA
#
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.13.1)

find_package(Zephyr HINTS $ENV{ZEPHYR_BASE})

project(app)
add_definitions(-Wno-packed-bitfield-compat)

zephyr_include_directories(${NRFXLIB_DIR}/zboss/include)
zephyr_include_directories(${NRFXLIB_DIR}/zboss/include/osif)
target_include_directories(app BEFORE PRIVATE mock)

target_sources(app PRIVATE
  ${NRF_DIR}/subsys/zigbee/osif/zb_nrf_logger.c
  src/main.c
  mock/log_mock.c
)

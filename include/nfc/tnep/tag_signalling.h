/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: LicenseRef-Nordic-5-Clause
 */

#ifndef NFC_TNEP_TAG_SIGNALLING_H_
#define NFC_TNEP_TAG_SIGNALLING_H_

#include <stdbool.h>

enum tnep_event {
	TNEP_EVENT_DUMMY,
	TNEP_EVENT_MSG_RX_NEW,
	TNEP_EVENT_TAG_SELECTED,
};

void nfc_tnep_tag_signalling_rx_msg_event_raise(enum tnep_event event);

void nfc_tnep_tag_signalling_tx_msg_event_raise(enum tnep_event event);

bool nfc_tnep_tag_signalling_rx_msg_event_check_and_clear(enum tnep_event *event);

bool nfc_tnep_tag_signalling_tx_msg_event_check_and_clear(enum tnep_event *event);

#endif /* NFC_TNEP_TAG_SIGNALLING_H_ */

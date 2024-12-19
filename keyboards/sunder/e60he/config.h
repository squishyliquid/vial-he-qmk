// Copyright 2023 squishyliquid (@squishyliquid)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define DEBUG_MATRIX_SCAN_RATE //
#define ADC_RESOLUTION ADC_CFGR1_RES_12BIT

// #define SPLIT_HAND_PIN GP11
// #define SPLIT_USB_DETECT
// #define SPLIT_USB_TIMEOUT 500
// #define SPLIT_USB_TIMEOUT_POLL 10
// #define SPLIT_WATCHDOG_ENABLE

#define SERIAL_USART_FULL_DUPLEX   // Enable full duplex operation mode.
#define SERIAL_USART_TX_PIN GP0    // USART TX pin
#define SERIAL_USART_RX_PIN GP1    // USART RX pin

#define BOOTMAGIC_ROW 0
#define BOOTMAGIC_COLUMN 8
#define BOOTMAGIC_ROW_RIGHT 3
#define BOOTMAGIC_COLUMN_RIGHT 8

#define BOOTMAGIC_ROW_2 0
#define BOOTMAGIC_COLUMN_2 9
#define BOOTMAGIC_ROW_RIGHT_2 4
#define BOOTMAGIC_COLUMN_RIGHT_2 9

// #define MATRIX_ROWS 6
// #define MATRIX_COLS 10
#define MUX_BITS 4

// #define MATRIX_ROW_PINS { GP26, GP27, GP28 }
// #define MATRIX_COL_PINS { NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN, NO_PIN }
#define ALL_MUX_PINS { GP18, GP19, GP20, GP21 }
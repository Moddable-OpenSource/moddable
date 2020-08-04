/**
 * Copyright (c) 2016 - 2018, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef PCA10056_H
#define PCA10056_H

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_gpio.h"

#define LEDS_NUMBER    2
#define LED_PRIMARY_PIN		7
#define LED_SECONDARY_PIN	14
#define LED_STATE_ON		0

#define LED_1          NRF_GPIO_PIN_MAP(0,7)
#define LED_2          NRF_GPIO_PIN_MAP(0,14)
#define LED_START      LED_1
#define LED_STOP       LED_2

#define LEDS_ACTIVE_STATE 1

#define LEDS_LIST { LED_1, LED_2 }

#define LEDS_INV_MASK  LEDS_MASK

#define BSP_LED_0      7
#define BSP_LED_1      7

// added Moddable Four encoder switch as BUTTON_2
#define BUTTONS_NUMBER 2
#define BUTTON_1       13
#define BUTTON_2       26

#define BUTTON_DFU		BUTTON_1
#define BUTTON_PULL    NRF_GPIO_PIN_PULLUP

#define BUTTONS_ACTIVE_STATE 0

#define BUTTONS_LIST { BUTTON_1, BUTTON_2 }

#define BSP_BUTTON_0   BUTTON_1
#define BSP_BUTTON_1   BUTTON_2

#define RX_PIN_NUMBER  31
#define TX_PIN_NUMBER  30
#define CTS_PIN_NUMBER 0
#define RTS_PIN_NUMBER 0
#define HWFC           false

#define BLEDIS_MANUFACTURER	"Moddable Tech, Inc."
#define BLEDIS_MODEL		"Moddable Four"

#define UF2_PRODUCT_NAME	"Moddable Four"
#define UF2_VOLUME_LABEL	"MODDABLE4  "
#define UF2_BOARD_ID		"nRF52840-m4-v1"
#define UF2_INDEX_URL		"https://www.moddable.com/product.php"

#define MODDABLE_4_ENCODER_A	6
#define MODDABLE_4_ENCODER_SW	26
#define MODDABLE_4_ENCODER_B	27
#define MODDABLE_4_LED			7
#define MODDABLE_4_SDA_PIN		8
#define MODDABLE_4_SCL_PIN		11
#define MODDABLE_4_BUTTON		13
#define MODDABLE_4_DISP			15
#define MODDABLE_4_CS			18
#define MODDABLE_4_SCK			19
#define MODDABLE_4_MOSI			21
#define MODDABLE_4_MISO			29
#define MODDABLE_4_LCD_POWER	23
#define MODDABLE_4_DBG_TX		30
#define MODDABLE_4_DBG_RX		31

//#define USB_DESC_VID           0x1915
//#define USB_DESC_UF2_PID       0x520F
#define USB_DESC_VID           0xbeef
#define USB_DESC_UF2_PID       0xcafe
#define USB_DESC_CDC_ONLY_PID  0x002A

#ifdef __cplusplus
}
#endif

#endif // PCA10056_H

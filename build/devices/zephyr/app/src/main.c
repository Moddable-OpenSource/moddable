/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdio.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>

#include "xsmc.h"
#include "xsHost.h"
#include "xsHosts.h"

#include "modInstrumentation.h"

#include "modTimer.h"
void modTimersExecute(void);
int modTimersNext(void);

/* 1000 msec = 1 sec */
#define SLEEP_TIME_MS   1000

/* The devicetree node identifier for the "led0" alias. */
// #define LED0_NODE DT_ALIAS(led0)


static void startMachine(void)
{
	xsMachine *the = modCloneMachine(NULL, NULL);

	modRunMachineSetup(the);

	while(1) {
uint32_t ms;
		modTimersExecute();
ms = modTimersNext();
printf("timernext: %d\n", ms);
modMessageService(the, ms);
//		modMessageService(the, modTimersNext());
		modInstrumentationAdjust(Turns, +1);
	}

printf("      -- end\n");
	xsDeleteMachine(the);
}

/*
 * A build error on this line means your board is unsupported.
 * See the sample documentation for information on how to fix this.
 */
// static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
	int ret;
	bool led_state = true;

	startMachine();

#if 0
	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}


	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return 0;
		}

		led_state = !led_state;
		printf("LED state: %s\n", led_state ? "ON" : "OFF");
		k_msleep(SLEEP_TIME_MS);
	}
#endif
	return 0;
}

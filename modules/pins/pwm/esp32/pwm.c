#include "xsmc.h"
#include "mc.xs.h"			// for xsID_ values
#include "mc.defines.h"

#include "driver/ledc.h"

#ifndef MODDEF_PWM_LEDC_CHANNEL
	#define MODDEF_PWM_LEDC_CHANNEL LEDC_CHANNEL_0
#endif
#ifndef MODDEF_PWM_LEDC_TIMER
	#define MODDEF_PWM_LEDC_TIMER LEDC_TIMER_0
#endif

static const ledc_timer_config_t gTimer = {
	.duty_resolution = LEDC_TIMER_10_BIT,
	.freq_hz = 1024,
	.speed_mode = LEDC_HIGH_SPEED_MODE,
	.timer_num = MODDEF_PWM_LEDC_TIMER
};

void xs_pwm_destructor(void *data)
{
	uintptr_t gpio = (uintptr_t)data;

	if (gpio == (uintptr_t)-1)
		return;

	ledc_set_duty(LEDC_HIGH_SPEED_MODE, MODDEF_PWM_LEDC_CHANNEL, gpio);
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, MODDEF_PWM_LEDC_CHANNEL);
}

void xs_pwm(xsMachine *the)
{
	static uint8_t initialized = false;
	int gpio;
	ledc_channel_config_t ledc;

	xsmcSetHostData(xsThis, (void *)(uintptr_t)-1);

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_pin);
	gpio = xsmcToInteger(xsVar(0));

	if (xsmcHas(xsArg(0), xsID_port))
		xsUnknownError("no port - esp32");

	if (!initialized) {
		if (ESP_OK != ledc_timer_config(&gTimer))
			xsUnknownError("configure ledc timer failed");
		initialized = true;
	}
	
	ledc.channel    = MODDEF_PWM_LEDC_CHANNEL;
	ledc.duty       = 0;
	ledc.gpio_num   = gpio;
	ledc.speed_mode = LEDC_HIGH_SPEED_MODE;
	ledc.timer_sel  = MODDEF_PWM_LEDC_TIMER;

	if (ESP_OK != ledc_channel_config(&ledc))
		xsUnknownError("configure ledc channel failed");

	xsmcSetHostData(xsThis, (void *)(uintptr_t)gpio);
}

void xs_pwm_close(xsMachine *the)
{
	xs_pwm_destructor(xsmcGetHostData(xsThis));
	xsmcSetHostData(xsThis, (void *)(uintptr_t)-1);
}

void xs_pwm_write(xsMachine *the)
{
	uintptr_t gpio = (uintptr_t)xsmcGetHostData(xsThis);
	int value = xsmcToInteger(xsArg(0));	// 0 to 1023

	if ((value < 0) || (value >= gTimer.freq_hz))
		xsRangeError("bad value");

	ledc_set_duty(LEDC_HIGH_SPEED_MODE, MODDEF_PWM_LEDC_CHANNEL, value);
	ledc_update_duty(LEDC_HIGH_SPEED_MODE, MODDEF_PWM_LEDC_CHANNEL);
}

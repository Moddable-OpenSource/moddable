/*
 * Copyright (c) 2025  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xsmc.h"
#include "xsHost.h"
#include "mc.devicetree.h"
#include "mc.xs.h"		// for xsID_ values
#include "builtinCommon.h"

#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>

#if kModZephyrRTCBusCount

typedef struct {
	const struct device *device;
	xsMachine	*the;
	xsSlot		obj;
	xsSlot		*onAlarm;
} xsRTCRecord, *xsRTC;

void xs_rtc_destructor(void *data)
{
	xsRTC xr = data;
	if (!xr) return;

	rtc_alarm_set_callback(xr->device, 0, NULL, NULL);
	c_free(data);
}

static void xs_rtc_mark(xsMachine* the, void *it, xsMarkRoot markRoot)
{
	xsRTC xr = (xsRTC)it;

	if (xr->onAlarm)
		(*markRoot)(the, xr->onAlarm);
}

static const xsHostHooks xsRTCHooks = {
	xs_rtc_destructor,
	xs_rtc_mark,
	NULL
};

void xs_rtc_constructor(xsMachine *the)
{
	xsSlot *onAlarm = builtinGetCallback(the, xsID_onAlarm);

	builtinInitializeTarget(the);

	xsmcVars(1);

	xsmcGet(xsVar(0), xsArg(0), xsID_port);
	const struct modZephyrRTC *rtc = modZephyrGetRTC(xsmcToString(xsVar(0)));
	if (!rtc)
		xsUnknownError("invalid port");
	if (!device_is_ready(rtc->device))
		xsUnknownError("rtc not ready");

	xsRTC xr = c_calloc(1, sizeof(xsRTCRecord));
	if (!xr)
		xsUnknownError("no memory");

	xr->device = rtc->device;
	xr->onAlarm = onAlarm;
	xr->the = the;
	xr->obj = xsThis;
	xsRemember(xr->obj);

	xsmcSetHostData(xsThis, xr);
	xsSetHostHooks(xsThis, &xsRTCHooks);
}

void xs_rtc_close(xsMachine *the)
{
	xsRTC xr = xsmcGetHostData(xsThis);
	if (xr && xsmcGetHostDataValidate(xsThis, (void *)&xsRTCHooks)) {
		xsForget(xr->obj);
		xs_rtc_destructor(xr);
		xsmcSetHostData(xsThis, NULL);
		xsSetHostDestructor(xsThis, NULL);
	}
}

void xs_rtc_time_get(xsMachine *the)
{
	xsRTC xr = xsmcGetHostDataValidate(xsThis, (void *)&xsRTCHooks);
	struct rtc_time rtm;
	if (rtc_get_time(xr->device, &rtm) < 0)
		return;		// time not available

	time_t seconds = timeutil_timegm((struct tm *)&rtm);
	xsmcSetNumber(xsResult, ((xsNumberValue)seconds * 1000) + (rtm.tm_nsec / 1000000));
}

void xs_rtc_time_set(xsMachine *the)
{
	xsRTC xr = xsmcGetHostDataValidate(xsThis, (void *)&xsRTCHooks);
	struct rtc_time rtm = {0};
	xsNumberValue ms = xsmcToNumber(xsArg(0));
	time_t seconds = (time_t)(ms / 1000);

	struct tm *tm_time = gmtime(&seconds);
	if (NULL == tm_time)
		xsUnknownError("bad value");

	*(struct tm *)&rtm = *tm_time;
	rtm.tm_nsec = (((int)ms) % 1000) * 1000000;

	if (rtc_set_time(xr->device, &rtm) < 0)
		xsUnknownError("failed");
}

static void rtcDeliver(void *the, void *refcon, uint8_t *, uint16_t)
{
	xsRTC xr = (xsRTC)refcon;

	xsBeginHost(the);
		xsCallFunction0(xsReference(xr->onAlarm), xr->obj);
	xsEndHost(the);
}

static void rtcCallback(const struct device *dev, uint16_t id, void *user_data)
{
	xsRTC xr = user_data;
	modMessagePostToMachineFromISR(xr->the, rtcDeliver, xr);
}

void xs_rtc_alarm(xsMachine *the)
{
	xsRTC xr = xsmcGetHostDataValidate(xsThis, (void *)&xsRTCHooks);

	struct rtc_time rtm = {0};
	xsNumberValue ms = xsmcToNumber(xsArg(0));
	time_t seconds = (time_t)(ms / 1000);

	struct tm *tm_time = gmtime(&seconds);
	if (NULL == tm_time)
		xsUnknownError("bad value");

	*(struct tm *)&rtm = *tm_time;

	uint16_t mask = RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_MONTHDAY;		// default for devices that don't provide supported fields
	rtc_alarm_get_supported_fields(xr->device, 0, &mask);
	mask &= ~RTC_ALARM_TIME_MASK_WEEKDAY;
	if (rtc_alarm_set_time(xr->device, 0, mask, &rtm) < 0)
		xsUnknownError("can't activate alarm");
	rtc_alarm_set_callback(xr->device, 0, rtcCallback, xr);
}

#else /* !kModZephyrRTCBusCount - no RTC available on this board */

void xs_rtc_destructor(void *data) {}
void xs_rtc_constructor(xsMachine *the) {xsUnknownError("unavailable");}
void xs_rtc_close(xsMachine *the) {}
void xs_rtc_time_get(xsMachine *the) {}
void xs_rtc_time_set(xsMachine *the) {}
void xs_rtc_alarm(xsMachine *the) {}

#endif

/*
 * Copyright (c) 2025-2026  Moddable Tech, Inc.
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
#include "mc.xs.h"			// for xsID_ values
#include "moddableAppState.h"
#include <pbl/logging/logging.h>
#include "applib/event_service_client.h"
#include "applib/ui/window_private.h"
#include "applib/ui/window_stack.h"
#include "applib/ui/app_window_stack.h"

#define kRecognizerSingle (1)
#define kRecognizerLong (2)
#define kRecognizerMulti (4)
#define kRecognizerRaw (8)

struct PebbleButtonRecord {
	struct PebbleButtonRecord	*next;

	xsMachine						*the;
	xsSlot	 						obj;
	xsSlot	 						*onPush;
	uint32_t							buttons;
	uint8_t							recognizers;
	int								repeat;
	int								delay;
	int								min;
	int								max;
	int								lastOnly;
	int								timeout;
};
typedef struct PebbleButtonRecord PebbleButtonRecord;
typedef struct PebbleButtonRecord *PebbleButton;

static void buttonClickProvider(void *unused);

static void buttonEventHandler(int pushed, int button, uint8_t recognizer, int count, int repeat)
{
	char *buttonName;
	if (BUTTON_ID_BACK == button)
		buttonName = "back";
	else if (BUTTON_ID_DOWN == button)
		buttonName = "down";
	else if (BUTTON_ID_SELECT == button)
		buttonName = "select";
	else if (BUTTON_ID_UP == button)
		buttonName = "up";
	else
		return;

	char *recognizerName;
	if (kRecognizerSingle == recognizer)
		recognizerName = "single";
	else if (kRecognizerMulti == recognizer)
		recognizerName = "multi";
	else if (kRecognizerLong == recognizer)
		recognizerName = "long";
	else if (kRecognizerRaw == recognizer)
		recognizerName = "raw";
	else
		return;

	button = 1 << button;
	for (PebbleButton pb = getModdableAppState(buttons); pb; pb = pb->next) {
		if (!(pb->buttons & button) || !(pb->recognizers & recognizer))
			continue;

		xsBeginHost(pb->the);
			xsmcVars(5);
			xsmcSetInteger(xsVar(0), pushed);
			xsmcSetStringX(xsVar(1), buttonName);
			xsmcSetStringX(xsVar(2), recognizerName);
			xsmcSetInteger(xsVar(3), count);
			xsmcSetBoolean(xsVar(4), repeat);
			xsCallFunction5(xsReference(pb->onPush), pb->obj, xsVar(0), xsVar(1), xsVar(2), xsVar(3), xsVar(4));
		xsEndHost(pb->the);
	}
}

void xs_pebblebutton_destructor(void *data)
{
	PebbleButton pb = data, walker;
	if (!pb) return;

	PebbleButton *p = (PebbleButton *)&getModdableAppState(buttons);
	while (*p && *p != pb)
		p = &(*p)->next;
	if (*p)
		*p = pb->next;

	Window *w = app_window_stack_get_top_window();
	if (w)
		window_set_click_config_provider(w, buttonClickProvider);

	c_free(pb);
}

static void handleSingleClick(ClickRecognizerRef recognizer, void *context)
{
	ButtonId button = click_recognizer_get_button_id(recognizer);
	buttonEventHandler(1, button, kRecognizerSingle, click_number_of_clicks_counted(recognizer), click_recognizer_is_repeating(recognizer));
	if (BUTTON_ID_BACK == button) {
		buttonEventHandler(1, button, kRecognizerRaw, 1, 0);
		buttonEventHandler(0, button, kRecognizerRaw, 1, 0);
	}
}

static void handleLongClick(ClickRecognizerRef recognizer, void *context)
{
	buttonEventHandler(1, click_recognizer_get_button_id(recognizer), kRecognizerLong, click_number_of_clicks_counted(recognizer), click_recognizer_is_repeating(recognizer));
}

static void handleLongClickRelease(ClickRecognizerRef recognizer, void *context)
{
	buttonEventHandler(0, click_recognizer_get_button_id(recognizer), kRecognizerLong, 0, click_recognizer_is_repeating(recognizer));
}

static void handleMultiClick(ClickRecognizerRef recognizer, void *context)
{
	buttonEventHandler(1, click_recognizer_get_button_id(recognizer), kRecognizerMulti, click_number_of_clicks_counted(recognizer), 0);
}

static void handleRawClick(ClickRecognizerRef recognizer, void *context)
{
	buttonEventHandler(1, click_recognizer_get_button_id(recognizer), kRecognizerRaw, 1, 0);
}

static void handleRawClickRelease(ClickRecognizerRef recognizer, void *context)
{
	buttonEventHandler(0, click_recognizer_get_button_id(recognizer), kRecognizerRaw, 1, 0);
}

void buttonClickProvider(void *unused)
{
	Window *w = app_window_stack_get_top_window();
	if (!w)
		return;

	uint8_t hasBack = 0;
	PebbleButton head = getModdableAppState(buttons);
	for (int button = 0; button < NUM_BUTTONS; button++) {
		for (int recognizer = 1; recognizer <= kRecognizerRaw; recognizer <<= 1) {
			PebbleButton match = C_NULL;
			for (PebbleButton walker = head; walker; walker = walker->next) {
				if (!(walker->buttons & (1 << button)) || !(walker->recognizers & recognizer))
					continue;
				hasBack |= BUTTON_ID_BACK == button;
				match = walker;
				break;
			}
			if (match) {
				switch (recognizer) {
					case kRecognizerLong:
						window_long_click_subscribe(button, match->delay, handleLongClick, handleLongClickRelease);
						break;
					case kRecognizerMulti:
						window_multi_click_subscribe(button, match->min, match->max, match->timeout, match->lastOnly, handleMultiClick);
						break;
					case kRecognizerRaw:
						if (BUTTON_ID_BACK != button)
							window_raw_click_subscribe(button, handleRawClick, handleRawClickRelease, C_NULL);
						else
							window_single_click_subscribe(button, handleSingleClick);
						break;
					case kRecognizerSingle:
						if (BUTTON_ID_BACK != button)
							window_single_repeating_click_subscribe(button, match->repeat, handleSingleClick);
						else
							window_single_click_subscribe(button, handleSingleClick);
						break;
				}
			}
			else {
				switch (recognizer) {
					case kRecognizerLong: window_long_click_subscribe(button, 0, C_NULL, C_NULL); break;
					case kRecognizerMulti: window_multi_click_subscribe(button, 0, 0, 0, 0, C_NULL); break;
					case kRecognizerRaw: window_raw_click_subscribe(button, C_NULL, C_NULL, C_NULL); break;
					case kRecognizerSingle: window_single_repeating_click_subscribe(button, 0, C_NULL); break;
				}
			}
		}
	}

	window_set_overrides_back_button(w, hasBack ? 1 : 0);		//@@ work around bug with unsubscribe...
}

static void xs_pebblebutton_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	PebbleButton pb = it;
	(*markRoot)(the, pb->onPush);
}

static const xsHostHooks xsPebbleButtonHooks = {
	xs_pebblebutton_destructor,
	xs_pebblebutton_mark,
	NULL
};

uint32_t resolveButton(xsMachine *the, xsSlot *aType)
{
	ButtonId button;
	char *type = xsmcToString(*aType);
	if (c_strcmp(type, "back") == 0)
		button = BUTTON_ID_BACK;
	else if (c_strcmp(type, "down") == 0)
		button = BUTTON_ID_DOWN;
	else if (c_strcmp(type, "select") == 0)
		button = BUTTON_ID_SELECT;
	else if (c_strcmp(type, "up") == 0)
		button = BUTTON_ID_UP;
	else
		xsUnknownError("unknown button type");
	return 1 << button;
}

void xs_pebblebutton(xsMachine *the)
{
	uint32_t buttons = 0;

	xsmcVars(3);
	if (xsmcGet(xsVar(1), xsArg(0), xsID_type)) {
		buttons = resolveButton(the, &xsVar(1));
	}
	else {
		xsSlot tmp;
		xsmcGet(xsVar(0), xsArg(0), xsID_types);
		xsmcGet(tmp, xsVar(0), xsID_length);
		int count = xsmcToInteger(tmp);
		if (count <= 0)
			xsUnknownError("no buttons");
		while (count--) {
			xsmcGetIndex(xsVar(1), xsVar(0), count);
			buttons |= resolveButton(the, &xsVar(1));
		}

	}
	if (!xsmcGet(xsVar(0), xsArg(0), xsID_onPush))
		xsUnknownError("onPush required");

	uint8_t recognizers = 0;
	int repeat = 0, delay = 0, min = 0, max = 0, lastOnly = 0, timeout = 0;
	xsmcGet(xsVar(1), xsArg(0), xsID_single);
	if (xsmcTest(xsVar(1))) {
		recognizers |= kRecognizerSingle;
		if (xsReferenceType == xsmcTypeOf(xsVar(1))) {
			if (xsmcGet(xsVar(2), xsVar(1), xsID_repeat))
				repeat = xsmcToInteger(xsVar(2));
		}
	}
	xsmcGet(xsVar(1), xsArg(0), xsID_long);
	if (xsmcTest(xsVar(1))) {
		recognizers |= kRecognizerLong;
		if (buttons & (1 << BUTTON_ID_BACK))
			xsUnknownError("no long back");
		if (xsReferenceType == xsmcTypeOf(xsVar(1))) {
			if (xsmcGet(xsVar(2), xsVar(1), xsID_delay))
				delay = xsmcToInteger(xsVar(2));
		}
	}
	xsmcGet(xsVar(1), xsArg(0), xsID_multi);
	if (xsmcTest(xsVar(1))) {
		recognizers |= kRecognizerMulti;
		if (xsReferenceType == xsmcTypeOf(xsVar(1))) {
			if (xsmcGet(xsVar(2), xsVar(1), xsID_min))
				min = xsmcToInteger(xsVar(2));
			if (xsmcGet(xsVar(2), xsVar(1), xsID_max))
				max = xsmcToInteger(xsVar(2));
			if (xsmcGet(xsVar(2), xsVar(1), xsID_lastOnly))
				lastOnly = xsmcToBoolean(xsVar(2));
			if (xsmcGet(xsVar(2), xsVar(1), xsID_timeout))
				timeout = xsmcToInteger(xsVar(2));
		}
	}
	xsmcGet(xsVar(1), xsArg(0), xsID_raw);
	if (xsmcTest(xsVar(1)))
		recognizers |= kRecognizerRaw;

	if (!recognizers)
		recognizers = kRecognizerRaw;

	PebbleButton pb = c_calloc(1, sizeof(PebbleButtonRecord));
	if (!pb) xsUnknownError("no memory");

	xsmcSetHostData(xsThis, pb);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsPebbleButtonHooks);

	pb->the = the;
	pb->obj = xsThis;
	xsRemember(pb->obj);
	pb->buttons = buttons;
	pb->onPush = xsmcToReference(xsVar(0));
	pb->recognizers = recognizers;
	pb->repeat = repeat;
	pb->delay = delay;
	pb->min = min;
	pb->max = max;
	pb->lastOnly = lastOnly;
	pb->timeout = timeout;
	
	pb->next = getModdableAppState(buttons);
	setModdableAppState(buttons, pb);

 	Window *w = app_window_stack_get_top_window();
	if (w)
		window_set_click_config_provider(w, buttonClickProvider);
}

void xs_pebblebutton_close(xsMachine *the)
{
	PebbleButton pb = xsmcGetHostDataValidate(xsThis, (void *)&xsPebbleButtonHooks);
	xsForget(pb->obj);
	xs_pebblebutton_destructor(pb);

	xsmcSetHostData(xsThis, NULL);
	xsmcSetHostDestructor(xsThis, NULL);
}

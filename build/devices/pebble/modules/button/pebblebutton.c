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
#include "mc.xs.h"			// for xsID_ values
#include "moddableAppState.h"
#include "system/logging.h"
#include "applib/event_service_client.h"
#include "applib/ui/window_private.h"
#include "applib/ui/window_stack.h"
#include "applib/ui/app_window_stack.h"

struct PebbleButtonRecord {
	struct PebbleButtonRecord	*next;

	xsMachine						*the;
	xsSlot	 						obj;
	xsSlot	 						*onPush;
	uint32_t							buttons;
};
typedef struct PebbleButtonRecord PebbleButtonRecord;
typedef struct PebbleButtonRecord *PebbleButton;

static void buttonEventHandler(int pushed, int button)
{
	PebbleButton pb;
	char *name;

	if (BUTTON_ID_BACK == button)
		name = "back";
	else if (BUTTON_ID_DOWN == button)
		name = "down";
	else if (BUTTON_ID_SELECT == button)
		name = "select";
	else if (BUTTON_ID_UP == button)
		name = "up";
	else
		return;

	button = 1 << button;
	for (pb = getModdableAppState(buttons); pb; pb = pb->next) {
		if (!(pb->buttons & button))
			continue;

		xsBeginHost(pb->the);
			xsmcVars(2);
			xsmcSetInteger(xsVar(0), pushed);
			xsmcSetStringX(xsVar(1), name);
			xsCallFunction2(xsReference(pb->onPush), pb->obj, xsVar(0), xsVar(1));
		xsEndHost(pb->the);
	}
}

static void buttonDownEventHandler(PebbleEvent *e, void *context)
{
	buttonEventHandler(1, e->button.button_id);
}

static void buttonUpEventHandler(PebbleEvent *e, void *context)
{
	buttonEventHandler(0, e->button.button_id);
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

	if (NULL == getModdableAppState(buttons)) {
		event_service_client_unsubscribe(&getModdableAppState(eventServiceUp));
		event_service_client_unsubscribe(&getModdableAppState(eventServiceDown));
	}

	Window *w = app_window_stack_get_top_window();
	if (w) {
		for (walker = getModdableAppState(buttons); walker; walker = walker->next) {
			if (walker->buttons & (1 << BUTTON_ID_BACK))
				break;
		}
		if (C_NULL == walker)
			window_set_overrides_back_button(w, false);
	}

	c_free(pb);
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
	char *type = xsmcToString(xsVar(1));
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

	xsmcVars(2);
	if (xsmcHas(xsArg(0), xsID_type)) {
		xsmcGet(xsVar(1), xsArg(0), xsID_type);
		buttons = resolveButton(the, &xsVar(0));
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
			buttons |= resolveButton(the, &xsVar(0));
		}

	}
	if (!xsmcHas(xsArg(0), xsID_onPush))
		xsUnknownError("onPush required");
	xsmcGet(xsVar(0), xsArg(0), xsID_onPush);

	PebbleButton pb = c_calloc(1, sizeof(PebbleButtonRecord));
	if (!pb) xsUnknownError("no memory");

	xsmcSetHostData(xsThis, pb);
	xsSetHostHooks(xsThis, (xsHostHooks *)&xsPebbleButtonHooks);

	pb->the = the;
	pb->obj = xsThis;
	xsRemember(pb->obj);
	pb->buttons = buttons;
	pb->onPush = xsmcToReference(xsVar(0));
	
	if (NULL == getModdableAppState(buttons)) {
		EventServiceInfo *i = &getModdableAppState(eventServiceDown);
		i->type = PEBBLE_BUTTON_DOWN_EVENT;
		i->handler = buttonDownEventHandler;
		event_service_client_subscribe(i);

		i = &getModdableAppState(eventServiceUp);
		i->type = PEBBLE_BUTTON_UP_EVENT;
		i->handler = buttonUpEventHandler;
		event_service_client_subscribe(i);
	}

	pb->next = getModdableAppState(buttons);
	setModdableAppState(buttons, pb);

	if ((1 << BUTTON_ID_BACK) & buttons)
		window_set_overrides_back_button(app_window_stack_get_top_window(), true);
}

void xs_pebblebutton_close(xsMachine *the)
{
	PebbleButton pb = xsmcGetHostDataValidate(xsThis, (void *)&xsPebbleButtonHooks);
	xsForget(pb->obj);
	xs_pebblebutton_destructor(pb);

	xsmcSetHostData(xsThis, NULL);
	xsmcSetHostDestructor(xsThis, NULL);
}

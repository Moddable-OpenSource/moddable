#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_ values
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
	ButtonId							button;
};
typedef struct PebbleButtonRecord PebbleButtonRecord;
typedef struct PebbleButtonRecord *PebbleButton;

static PebbleButton gButtons;
static EventServiceInfo	eventServiceDown;
static EventServiceInfo	eventServiceUp;

static void buttonEventHandler(int pushed, int button)
{
	PebbleButton pb;

	for (pb = gButtons; pb; pb = pb->next) {
		if (pb->button != button)
			continue;

		xsBeginHost(pb->the);
		xsCallFunction1(xsReference(pb->onPush), pb->obj, xsInteger(pushed));
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

	PebbleButton *p = &gButtons;
	while (*p && *p != pb)
		p = &(*p)->next;
	if (*p)
		*p = pb->next;

	if (NULL == gButtons) {
		event_service_client_unsubscribe(&eventServiceUp);
		event_service_client_unsubscribe(&eventServiceDown);
	}
	
	for (walker = gButtons; walker; walker = walker->next) {
		if (walker->button == BUTTON_ID_BACK)
			break;
	}
	if (C_NULL == walker)
		window_set_overrides_back_button(app_window_stack_get_top_window(), false);

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

void xs_pebblebutton(xsMachine *the)
{
	ButtonId button;

	xsmcVars(1);
	xsmcGet(xsVar(0), xsArg(0), xsID_type);
	char *type = xsmcToString(xsVar(0));
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
	pb->button = button;
	pb->onPush = xsmcToReference(xsVar(0));
	
	if (NULL == gButtons) {
		eventServiceDown.type = PEBBLE_BUTTON_DOWN_EVENT;
		eventServiceDown.handler = buttonDownEventHandler;
		eventServiceUp.type = PEBBLE_BUTTON_UP_EVENT;
		eventServiceUp.handler = buttonUpEventHandler;
		event_service_client_subscribe(&eventServiceUp);
		event_service_client_subscribe(&eventServiceDown);
	}

	pb->next = gButtons;
	gButtons = pb;

	if (BUTTON_ID_BACK == button)
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

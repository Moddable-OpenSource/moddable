#include "xsmc.h"
#include "xsHost.h"
#include "mc.xs.h"			// for xsID_ values


#if 1
	#include "applib/event_service_client.h"
#else

typedef enum {
  PEBBLE_NULL_EVENT = 0,
  PEBBLE_ACCEL_SHAKE_EVENT,
  PEBBLE_ACCEL_DOUBLE_TAP_EVENT,
  PEBBLE_BT_CONNECTION_EVENT,
  PEBBLE_BT_CONNECTION_DEBOUNCED_EVENT,
  PEBBLE_BUTTON_DOWN_EVENT,
  PEBBLE_BUTTON_UP_EVENT
} PebbleEventType;

typedef enum {
  //! Back button
  BUTTON_ID_BACK = 0,
  //! Up button
  BUTTON_ID_UP,
  //! Select (middle) button
  BUTTON_ID_SELECT,
  //! Down button
  BUTTON_ID_DOWN,
  //! Total number of buttons
  NUM_BUTTONS
} ButtonId;

typedef struct {
	ButtonId button_id;
 } PebbleButtonEvent;
 
 typedef struct  {
	union  {
	  PebbleButtonEvent button;
	};
	PebbleEventType type;
 } PebbleEvent;

 typedef void (*EventServiceEventHandler)(PebbleEvent *e, void *context);

 typedef struct {
	PebbleEventType type;
	EventServiceEventHandler handler;
	void *context;
 } EventServiceInfo;

static void event_service_client_subscribe(EventServiceInfo * service_info) {}
static void event_service_client_unsubscribe(EventServiceInfo * service_info) {}
 #endif

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

static void buttonEventHandler(PebbleEvent *e, void *context)
{
	PebbleButton pb;

	for (pb = gButtons; pb; pb = pb->next) {
		if (pb->button != e->button.button_id)
			continue;

		xsMachine *the = pb->the;
		xsBeginHost(the);
		xsCallFunction1(xsReference(pb->onPush), pb->obj, xsInteger((e->type == PEBBLE_BUTTON_UP_EVENT) ? 0 : 1));
		xsEndHost(the);
	}
}

void xs_pebblebutton_destructor(void *data)
{
	PebbleButton pb = data;
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
		eventServiceDown.handler = buttonEventHandler;
		eventServiceUp.type = PEBBLE_BUTTON_UP_EVENT;
		eventServiceUp.handler = buttonEventHandler;
		event_service_client_subscribe(&eventServiceUp);
		event_service_client_subscribe(&eventServiceDown);
	}

	pb->next = gButtons;
	gButtons = pb;
}

void xs_pebblebutton_close(xsMachine *the)
{
	PebbleButton pb = xsmcGetHostDataValidate(xsThis, (void *)&xsPebbleButtonHooks);
	xsForget(pb->obj);
	xs_pebblebutton_destructor(pb);

	xsmcSetHostData(xsThis, NULL);
	xsmcSetHostDestructor(xsThis, NULL);
}

#if 0
void xs_pebblebutton_trigger(xsMachine *the)
{
	PebbleButton pb = xsmcGetHostDataValidate(xsThis, (void *)&xsPebbleButtonHooks);
	PebbleEvent e;
	e.button.button_id = pb->button;
	e.type = xsmcTest(xsArg(0)) ? PEBBLE_BUTTON_DOWN_EVENT : PEBBLE_BUTTON_UP_EVENT;
	buttonEventHandler(&e, NULL);
}
#endif

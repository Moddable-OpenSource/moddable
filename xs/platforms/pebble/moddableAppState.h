#include "process_state/app_state/app_state.h"
#include "services/common/evented_timer.h"
#include "applib/event_service_client.h"

#include "xs.h"

typedef struct {
	xsMachine				*the;

	// pebblebutton
	void					*buttons;
	EventServiceInfo		eventServiceDown;
	EventServiceInfo		eventServiceUp;

	// pebble-accelerometer
	void					*accelerometer;

	// pebble-battery
	void					*battery;

	// timer
	void					*timers;
	EventedTimerID		eventedTimer;
} ModdablePebbleAppStateRecord, *ModdablePebbleAppState;

#define getModdableAppState(FIELD) (((ModdablePebbleAppState)app_state_get_rocky_memory_api_context())->FIELD)
#define setModdableAppState(FIELD, VALUE) ((ModdablePebbleAppState)app_state_get_rocky_memory_api_context())->FIELD = VALUE

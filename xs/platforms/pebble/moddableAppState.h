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

#include "process_state/app_state/app_state.h"
#include "services/evented_timer.h"
#include "applib/event_service_client.h"

#include "xs.h"

struct DebugFragmentRecord {
	struct DebugFragmentRecord *next;
	uint16_t remaining;
	uint16_t offset;
	uint8_t bytes[];
};
typedef struct DebugFragmentRecord DebugFragmentRecord;
typedef struct DebugFragmentRecord *DebugFragment;

typedef struct {
	xsMachine				*the;

	// creation flags from ModdableCreationRecord
	uint32_t				creationFlags;

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

	// pebble-compass
	void					*compass;

	// pebble-appmessage
	void					*appMessage;
	uint8_t				pkjsReady;
	uint8_t				notFirst;

	// pebble-files
	char					*root;

	// app-focus service
	uint8_t				willFocus:1;
	uint8_t				didFocus:1;

	// XS FFI
	void					*fxBuildFFI;

	DebugFragment		debugFragments;
	char					*abortReason;
} ModdablePebbleAppStateRecord, *ModdablePebbleAppState;

#define getModdableAppState(FIELD) (((ModdablePebbleAppState)app_state_get_js_memory_api_context())->FIELD)
#define setModdableAppState(FIELD, VALUE) ((ModdablePebbleAppState)app_state_get_js_memory_api_context())->FIELD = VALUE

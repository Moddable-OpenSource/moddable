/*
 * Copyright (c) 2016-2018  Moddable Tech, Inc.
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

// extern uint8_t _MODPREF_start;
// extern uint8_t _MODPREF_end;

#define kPreferencesMagic 0x81213141

enum {
	kPrefsTypeBoolean = 1,
	kPrefsTypeInteger = 2,
	kPrefsTypeString = 3,
	kPrefsTypeBuffer = 4,
};

#define kBufferSize (64)

static void resetPrefs(void);
static uint8_t findPrefsBlock(uint32_t *offset);
static uint8_t findPrefOffset(const char *domain, const char *key, uint32_t *entryOffset, uint32_t *valueOffset, uint32_t *entrySize, uint8_t *buffer);
static int getPrefSize(const uint8_t *pref);
static uint8_t erasePref(const char *domain, const char *key, uint8_t *buffer);
static uint8_t setPref(xsMachine *the, char *domain, char *name, uint8_t type, uint8_t *value, uint16_t byteCount);

void xs_preference_set(xsMachine *the)
{
	xsUnknownError("can't save prefs");
}

void xs_preference_get(xsMachine *the)
{
}

void xs_preference_delete(xsMachine *the)
{
}

void xs_preference_keys(xsMachine *the)
{
}

void xs_preference_reset(xsMachine *the)
{
}


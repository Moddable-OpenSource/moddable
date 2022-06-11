/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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

#include "piuAll.h"
#include "mc.defines.h"
#ifndef MODDEF_LOCALS_ALL
	#define MODDEF_LOCALS_ALL (false)
#endif

extern void fxStringX(xsMachine* the, xsSlot* theSlot, xsStringValue theValue);

#define mxLocalsNameSize 16
#define mxLocalsLanguageSize 16
typedef struct PiuLocalsStruct PiuLocalsRecord, *PiuLocals;
struct PiuLocalsStruct {
	PiuHandlePart;
	int32_t* seeds;
	int32_t* results;
	int32_t length;
	char name[mxLocalsNameSize];
	char language[mxLocalsLanguageSize];
};

static uint32_t PiuLocalsHash(uint32_t d, uint8_t* string);
static void PiuLocalsMark(xsMachine* the, void* it, xsMarkRoot markRoot);

static const xsHostHooks ICACHE_FLASH_ATTR PiuLocalsHooks = {
	PiuLocalsDelete,
	PiuLocalsMark,
	NULL
};

void PiuLocalsCreate(xsMachine* the)
{
	xsIntegerValue c = xsToInteger(xsArgc);
	PiuLocals* self;
	char path[mxLocalsNameSize + 1 + mxLocalsLanguageSize + 4 + 1];
	size_t size;
	xsSetHostChunk(xsThis, NULL, sizeof(PiuLocalsRecord));
	self = PIU(Locals, xsThis);
	(*self)->reference = xsToReference(xsThis);
	xsSetHostHooks(xsThis, &PiuLocalsHooks);
	if ((c > 0) && xsTest(xsArg(0))) {
		xsStringValue string = xsToString(xsArg(0));
		if (c_strlen(string) > mxLocalsNameSize - 1)
			xsRangeError("locals name too long: %s", string);	
		c_strcpy((*self)->name, string);
	}
	else
		c_strcpy((*self)->name, "locals");
	if ((c > 1) && xsTest(xsArg(1))) {
		xsStringValue string = xsToString(xsArg(0));
		if (c_strlen(string) > mxLocalsLanguageSize - 1)
			xsRangeError("locals language too long: %s", string);	
		c_strcpy((*self)->language, string);
	}
	else
		c_strcpy((*self)->language, "en");
	c_strcpy(path, (*self)->name);
	c_strcat(path, ".mhi");
	(*self)->seeds = (int32_t *)fxGetResource(the, NULL, path, &size);
	if (!(*self)->seeds) {
		xsURIError("locals name not found: %s", path);	
	}
	c_strcpy(path, (*self)->name);
	c_strcat(path, ".");
	c_strcat(path, (*self)->language);
	c_strcat(path, ".mhr");
	(*self)->results = (int32_t *)fxGetResource(the, NULL, path, &size);
	if (!(*self)->results) {
		xsURIError("locals language not found: %s", path);	
	}
	(*self)->length = c_read32((*self)->seeds);
}

void PiuLocalsDelete(void* it)
{
}

uint32_t PiuLocalsHash(uint32_t d, uint8_t* s)
{
	int32_t c;
	if (d == 0)
		d = 0x811c9dc5;
    while ((c = c_read8(s))) {
		d *= 0x01000193;
		d ^= c;
		s++;
    }
    return (d & 0x7FFFFFFF);
}

void PiuLocalsMark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
}

void PiuLocals_get_language(xsMachine* the)
{
	PiuLocals* self = PIU(Locals, xsThis);
	xsResult = xsString((*self)->language);
}

void PiuLocals_set_language(xsMachine* the)
{
	PiuLocals* self = PIU(Locals, xsThis);
	char path[mxLocalsNameSize + 1 + mxLocalsLanguageSize + 4 + 1];
	size_t size;
	xsStringValue string = xsToString(xsArg(0));
	if (c_strlen(string) > mxLocalsLanguageSize - 1)
		xsRangeError("locals language too long: %s", string);	
	c_strcpy((*self)->language, string);
	c_strcpy(path, (*self)->name);
	c_strcat(path, ".");
	c_strcat(path, (*self)->language);
	c_strcat(path, ".mhr");
	(*self)->results = (int32_t *)fxGetResource(the, NULL, path, &size);
	if (!(*self)->results)
		xsURIError("locals language not found: %s", path);	
}

void PiuLocals_get(xsMachine* the)
{
	PiuLocals* self = PIU(Locals, xsThis);
	xsStringValue string = xsToString(xsArg(0));
	int32_t* seeds = (*self)->seeds;
	int32_t* results = (*self)->results;
	if (seeds && results) {
		int32_t c = (*self)->length;
		int32_t d = (int32_t)PiuLocalsHash(0, (uint8_t*)string) % c;
		int32_t offset;
		d = c_read32(seeds + 1 + d);
		if (d < 0) 
			d = 0 - d - 1;
		else if (d > 0) 
			d = (int32_t)PiuLocalsHash((uint32_t)d, (uint8_t*)string) % c;
		else
			return;
#if !MODDEF_LOCALS_ALL
		offset = c_read32(seeds + 1 + c + 1 + d);
  		if (c_strcmp((xsStringValue)seeds + offset, string))
  			return;
#endif
  		offset = c_read32(results + 1 + d);
  		string = (xsStringValue)results + offset;
  		fxStringX(the, &xsResult, string);
	}
}

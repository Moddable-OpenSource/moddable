/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
#include "mc.xs.h"      // for xsID_ values
#include "builtinCommon.h"

#include "applib/voice/dictation_session.h"

typedef struct {
	xsMachine	*the;
	xsSlot		obj;
	
	xsSlot		*onReadable;
	xsSlot		*onError;

	DictationSession *session;
	
	char		*transcription;
} xsDictationRecord, *xsDictation;

static void xs_dictation_mark(xsMachine* the, void* it, xsMarkRoot markRoot);
static void dictationCallback(DictationSession *session, DictationSessionStatus status, char *transcription, void *context);


static const xsHostHooks ICACHE_RODATA_ATTR xsDictationHooks = {
	xs_dictation_destructor,
	xs_dictation_mark,
	NULL
};

void xs_dictation_destructor(void *data)
{
	xsDictation dt = data;
	if (dt) {
		if (dt->session)
			dictation_session_destroy(dt->session);
		if (dt->transcription)
			c_free(dt->transcription);
		c_free(dt);
	}
}

void xs_dictation_mark(xsMachine* the, void* it, xsMarkRoot markRoot)
{
	xsDictation dt = it;

	if (dt->onError)
		(*markRoot)(the, dt->onError);
	if (dt->onReadable)
		(*markRoot)(the, dt->onReadable);
}

void xs_dictation(xsMachine *the)
{

	xsDictation dt = c_calloc(1, sizeof(xsDictationRecord));
	if (!dt)
		xsUnknownError("no memory");

	uint32_t byteLength = 0;
	if (xsmcHas(xsArg(0), xsID_byteLength)) {
		xsSlot tmp;
		xsmcGet(tmp, xsArg(0), xsID_byteLength);
		byteLength = xsmcToInteger(tmp);
	}
	dt->session = dictation_session_create(byteLength, dictationCallback, dt);
	if (C_NULL == dt->session) {
		c_free(dt);
		xsUnknownError("create session fail");
	}

	xsSetHostHooks(xsThis, (xsHostHooks *)&xsDictationHooks);

	xsmcSetHostData(xsThis, dt);
	dt->the = the;
	dt->obj = xsThis;
	xsRemember(dt->obj);
	dt->onReadable = builtinGetCallback(the, xsID_onReadable);
	dt->onError = builtinGetCallback(the, xsID_onError);
}

void xs_dictation_close(xsMachine *the)
{
	xsDictation dt = xsmcGetHostData(xsThis);
	if (dt && xsmcGetHostDataValidate(xsThis, (void *)&xsDictationHooks)) {
		xsForget(dt->obj);
		xs_dictation_destructor(dt);
		xsmcSetHostData(xsThis, NULL);
		xsmcSetHostDestructor(xsThis, NULL);
	}
}

void xs_dictation_configure(xsMachine *the)
{
	xsDictation dt = xsmcGetHostDataValidate(xsThis, (void *)&xsDictationHooks);
	xsSlot tmp;

	if (xsmcHas(xsArg(0), xsID_confirm)) {
		xsmcGet(tmp, xsArg(0), xsID_confirm);
		dictation_session_enable_confirmation(dt->session, xsmcToBoolean(tmp));
	}

	if (xsmcHas(xsArg(0), xsID_errorDialogs)) {
		xsmcGet(tmp, xsArg(0), xsID_errorDialogs);
		dictation_session_enable_error_dialogs(dt->session, xsmcToBoolean(tmp));
	}
}

void xs_dictation_start(xsMachine *the)
{
	xsDictation dt = xsmcGetHostDataValidate(xsThis, (void *)&xsDictationHooks);
	DictationSessionStatus status = dictation_session_start(dt->session);
	if (DictationSessionStatusSuccess != status)
		xsUnknownError("failed %d", status);
}


void xs_dictation_stop(xsMachine *the)
{
	xsDictation dt = xsmcGetHostDataValidate(xsThis, (void *)&xsDictationHooks);
	DictationSessionStatus status = dictation_session_stop(dt->session);
	if (DictationSessionStatusSuccess != status)
		xsUnknownError("failed %d", status);
}

void xs_dictation_read(xsMachine *the)
{
	xsDictation dt = xsmcGetHostDataValidate(xsThis, (void *)&xsDictationHooks);
	if (dt->transcription) {
		xsmcSetString(xsResult, dt->transcription);
		c_free(dt->transcription);
		dt->transcription = C_NULL;
	}
}

void dictationCallback(DictationSession *session, DictationSessionStatus status, char *transcription, void *context)
{
	xsDictation dt = context;
	xsMachine *the = dt->the;

	if (dt->transcription)
		c_free(dt->transcription);
	dt->transcription = C_NULL;

	xsBeginHost(the);

	if (DictationSessionStatusSuccess == status) {
		size_t byteLength = c_strlen(transcription);
		dt->transcription = c_malloc(byteLength + 1);
		if (C_NULL == dt->transcription) {
			status = DictationSessionStatusFailureInternalError;
			goto fail;
		}
		c_strcpy(dt->transcription, transcription);
		if (dt->onReadable)
			xsCallFunction1(xsReference(dt->onReadable), dt->obj, xsResult);
	}
	else if (dt->onError) {
fail:
		xsmcSetInteger(xsResult, status);
		xsCallFunction1(xsReference(dt->onError), dt->obj, xsResult);
	}

	xsEndHost(the);
}

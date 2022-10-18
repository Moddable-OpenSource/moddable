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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

#include "xsAll.h"
#if mxMacOSX
#include <mach/mach_time.h>
#define mxProfileTime() mach_absolute_time()
#endif

#ifdef mxProfile

typedef struct sxProfiler txProfiler;
typedef struct sxProfilerRecord txProfilerRecord;
typedef struct sxProfilerSample txProfilerSample;

struct sxProfiler {
	txU8 former;
	txU8 when;
	txU8 start;
	txU8 stop;
	txU4 delta;
	txU4 recordCount;
	txProfilerRecord** records;
	txU8 sampleCount;
	txU8 sampleIndex;
	txProfilerSample* samples;
	txProfilerRecord* host;
	txProfilerRecord* gc;
};

struct sxProfilerRecord {
	txU4 recordID;
	txID constructorID;
	txID prototypeID;
	txID functionID;
	txID file;
	txInteger line;
	txInteger ticks;
	txInteger calleeCount;
	txU4* callees;
};

struct sxProfilerSample {
	txU4 recordID;
	txU4 delta;
};

static void fxCreateProfiler(txMachine* the);
static void fxDeleteProfiler(txMachine* the);
static void fxPrintID(txMachine* the, FILE* file, txID id);
static void fxPrintProfiler(txMachine* the);
static void fxPushProfilerSample(txMachine* the, txU4 recordID, txU4 delta);
static txProfilerRecord* fxFrameToProfilerRecord(txMachine* the, txSlot* frame);
static void fxInsertProfilerCallee(txMachine* the, txProfilerRecord* record, txU4 recordID);
static txProfilerRecord* fxNewProfilerRecord(txMachine* the, txU4 recordIndex);

void fxCheckProfiler(txMachine* the, txSlot* frame)
{
	txProfiler* profiler = the->profiler;
	if (!profiler)
		return;
	txU8 when = profiler->when;
	txU8 time = mxProfileTime();
	if (when < time) {
		txProfilerRecord* callee = C_NULL;
		txProfilerRecord* record = C_NULL;
		if (frame) {
			record = fxFrameToProfilerRecord(the, frame);
			frame = frame->next;
		}
		else {
			frame = the->frame;
			if (frame)
				record = profiler->gc;
		}
		if (record) {
			callee = record;
			while (frame) {
				txProfilerRecord* parent = fxFrameToProfilerRecord(the, frame);
				if (parent) {
					fxInsertProfilerCallee(the, parent, callee->recordID);
					callee = parent;
				}
				frame = frame->next;
			}
		}
		if (callee)
			fxInsertProfilerCallee(the, profiler->host, callee->recordID);
		else
			record = profiler->host;
			
		txU8 delta = profiler->delta;
		txU8 former = profiler->former;

		record->ticks++;
		fxPushProfilerSample(the, record->recordID, when - former);
		when += delta;
		while (when < time) {
			record->ticks++;
			fxPushProfilerSample(the, record->recordID, delta);
			when += delta;
		}
			
		profiler->former = time;
		profiler->when = when;
	}
}

void fxCreateProfiler(txMachine* the)
{
	txProfiler* profiler = the->profiler = c_malloc(sizeof(txProfiler));
	if (profiler == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	profiler->delta = 1000;
	profiler->former = mxProfileTime();
	profiler->when = profiler->former + profiler->delta;
	profiler->start = profiler->former;
	
	profiler->recordCount = 2;
	profiler->records = (txProfilerRecord**)c_calloc(sizeof(txProfilerRecord*), profiler->recordCount);
	if (profiler->records == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
		
	profiler->sampleCount = 1024;
	profiler->sampleIndex = 0;
	profiler->samples = (txProfilerSample*)c_malloc(profiler->sampleCount * sizeof(txProfilerSample));
	if (profiler->samples == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	profiler->host = fxNewProfilerRecord(the, 0);
	profiler->gc = fxNewProfilerRecord(the, 1);
}

void fxDeleteProfiler(txMachine* the)
{
	txProfiler* profiler = the->profiler;
	c_free(profiler->samples);
	txU4 recordIndex = 0;
	while (recordIndex < profiler->recordCount) {
		txProfilerRecord* record = profiler->records[recordIndex];
		if (record) {
			if (record->callees)
				c_free(record->callees);
			c_free(record);
		}
		recordIndex++;
	}	
	c_free(profiler->records);
	c_free(profiler);
	the->profiler = C_NULL;
}

txProfilerRecord* fxFrameToProfilerRecord(txMachine* the, txSlot* frame)
{
	txProfiler* profiler = the->profiler;
	txSlot* function = frame + 3;
	if (function->kind == XS_REFERENCE_KIND) {
		function = function->value.reference;
		if (mxIsFunction(function)) {
			txSlot* profile = mxFunctionInstanceProfile(function);
			txU4 recordID = profile->value.profile.id;
			txProfilerRecord* record = C_NULL;
			if (recordID < profiler->recordCount)
				record = profiler->records[recordID];
			if (record)
				return record;
			record = fxNewProfilerRecord(the, recordID);
			record->functionID = mxFunctionInstanceCode(function)->ID;
			txSlot* home = mxFunctionInstanceHome(function)->value.home.object;
			if (home) {
				if (mxIsFunction(home)) {
					record->constructorID = mxFunctionInstanceCode(home)->ID;
				}
				else {
					txSlot* property = home->next;
					while (property && (property->flag & XS_INTERNAL_FLAG))
						property = property->next;
					while (property) {
						if (property->ID == mxID(_constructor))
							break;
						property = property->next;
					}
					if (property) {
						if (property->kind == XS_REFERENCE_KIND) {
							property = property->value.reference;
							if (mxIsFunction(property)) {
								record->constructorID = mxFunctionInstanceCode(property)->ID;
								record->prototypeID = mxID(_prototype);
							}
						}
					}
					else {
						property = home->next;
						while (property) {
							if (property->ID == mxID(_Symbol_toStringTag))
								break;
							property = property->next;
						}
						if (property) {
							if ((property->kind == XS_STRING_KIND) || (property->kind == XS_STRING_X_KIND)) {
								record->prototypeID = fxFindName(the, property->value.string);
							}
						}
					}
				}
			}
			record->file = profile->value.profile.file;
			record->line = profile->value.profile.line;
			return record;
		}
	}
	return C_NULL;
}

void fxInsertProfilerCallee(txMachine* the, txProfilerRecord* record, txU4 recordID)
{
	txInteger count = record->calleeCount;
	if (count == 0) {
		record->callees = c_malloc(sizeof(txU4));
		if (record->callees == C_NULL)
			fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
		record->calleeCount = 1;
		record->callees[0] = recordID;
		return;
	}
	txInteger cmp = -1;
    txInteger min = 0;
	txInteger max = count;
	txInteger mid;
	txU4* callee;
	while (min < max) {
		mid = (min + max) >> 1;
		callee = record->callees + mid;
		cmp = recordID - *callee;
		if (cmp < 0)
			max = mid;
		else if (cmp > 0)
			min = mid + 1;
		else
			return;
	}
	if (cmp > 0)
		mid++;
	record->calleeCount++;
	record->callees = c_realloc(record->callees, record->calleeCount * sizeof(txU4));
	if (record->callees == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	callee = record->callees + mid;
	if (mid < count)
		c_memmove(callee + 1, callee, (count - mid) * sizeof(txU4));
	*callee = recordID;
}

txProfilerRecord* fxNewProfilerRecord(txMachine* the, txU4 recordIndex)
{
	txProfiler* profiler = the->profiler;
	txU4 recordCount = profiler->recordCount;
	txProfilerRecord* record;
	if (recordIndex >= recordCount) {
		recordCount = recordIndex + 1;
		profiler->records = (txProfilerRecord**)c_realloc(profiler->records, recordCount * sizeof(txProfilerRecord*));
		if (profiler->records == C_NULL)
			fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
		c_memset(profiler->records + profiler->recordCount, 0, (recordCount - profiler->recordCount) * sizeof(txProfilerRecord*));
		profiler->recordCount = recordCount;
	}
	record = c_malloc(sizeof(txProfilerRecord));
	if (record == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	record->recordID = recordIndex;
	record->ticks = 0;
	record->constructorID = XS_NO_ID;
	record->prototypeID = XS_NO_ID;
	record->functionID = XS_NO_ID;
	record->file = XS_NO_ID;
	record->line = 0;
	record->calleeCount = 0;
	record->callees = C_NULL;
	profiler->records[recordIndex] = record;
	return record;
}

void fxPrintID(txMachine* the, FILE* file, txID id)
{
	txSlot* key = fxGetKey(the, id);
	if (key) {
		if ((key->kind == XS_KEY_KIND) || (key->kind == XS_KEY_X_KIND)) {
			fprintf(file, "%s", key->value.key.string);
			return;
		}
		if ((key->kind == XS_STRING_KIND) || (key->kind == XS_STRING_X_KIND)) {
			fprintf(file, "[%s]", key->value.string);
			return;
		}
	}
	fprintf(file, "?");
}

void fxPrintProfiler(txMachine* the)
{
	// https://chromedevtools.github.io/devtools-protocol/tot/Profiler/#type-Profile

	txProfiler* profiler = the->profiler;
	FILE* file;
    time_t timer;
    struct tm* tm_info;
    char buffer[22];
    char name[36];
    timer = time(NULL);
    tm_info = localtime(&timer);
    strftime(buffer, 22, "XS-%y-%m-%d-%H-%M-%S-", tm_info);
    sprintf(name, "%s%03llu.cpuprofile", buffer, (profiler->stop / 1000) % 1000);
	file = fopen(name, "w");
	
	fprintf(file, "{\"nodes\":[");
	txU4 recordIndex = 0;
	while (recordIndex < profiler->recordCount) {
		txProfilerRecord* record = profiler->records[recordIndex];
		if (record) {
			if (recordIndex > 0)
				fprintf(file, ",");
			fprintf(file, "{\"id\":%d,\"callFrame\":{\"functionName\":\"", record->recordID);
			if (record->functionID != XS_NO_ID) {
				if (record->constructorID != XS_NO_ID) {
					fxPrintID(the, file, record->constructorID);
					fprintf(file, ".");
				}
				if (record->prototypeID != XS_NO_ID) {
					fxPrintID(the, file, record->prototypeID);
					fprintf(file, ".");
				}
				fxPrintID(the, file, record->functionID);
			}
			else if (recordIndex == 0)
				fprintf(file, "(host)");
			else if (recordIndex == 1)
				fprintf(file, "(garbage collector)");

			fprintf(file, "\",\"scriptId\":\"0\",\"url\":\"");
			if (record->file != XS_NO_ID) {
				txSlot* key = fxGetKey(the, record->file);
				if (key) {
					fprintf(file, "%s", key->value.key.string);
				}
			}
			fprintf(file, "\",\"lineNumber\":%d,\"columnNumber\":-1},\"hitCount\":%d,\"children\":[", record->line - 1, record->ticks);
			txInteger calleeIndex = 0;
			while (calleeIndex < record->calleeCount) {
				if (calleeIndex > 0)
					fprintf(file, ",");
				fprintf(file, "%d", record->callees[calleeIndex]);
				calleeIndex++;
			}
			fprintf(file, "]}");
		}
		recordIndex++;
	}
	fprintf(file, "],\"startTime\":%lld,\"endTime\":%lld,\"samples\":[", profiler->start, profiler->when);
	{
		txU8 sampleIndex = 0;
		while (sampleIndex < profiler->sampleIndex) {
			txProfilerSample* sample = profiler->samples + sampleIndex;
			if (sampleIndex > 0)
				fprintf(file, ",");
			fprintf(file, "%d", sample->recordID);
			sampleIndex++;
		}
	}
	fprintf(file, "],\"timeDeltas\":[");
	{
		txU8 sampleIndex = 0;
		while (sampleIndex < profiler->sampleIndex) {
			txProfilerSample* sample = profiler->samples + sampleIndex;
			if (sampleIndex > 0)
				fprintf(file, ",");
			fprintf(file, "%d", sample->delta);
			sampleIndex++;
		}
	}
	fprintf(file, "]}");
	fclose(file);
}

void fxPushProfilerSample(txMachine* the, txU4 recordID, txU4 delta)
{
	txProfiler* profiler = the->profiler;
	txU8 sampleCount = profiler->sampleCount;
	txU8 sampleIndex = profiler->sampleIndex;
	txProfilerSample* sample;
	if (sampleIndex == sampleCount) {
		sampleCount += 1024;
		profiler->samples = (txProfilerSample*)c_realloc(profiler->samples, sampleCount * sizeof(txProfilerSample));
		if (profiler->samples == C_NULL)
			fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
		profiler->sampleCount = sampleCount;
	}
	sample = profiler->samples + sampleIndex;
	sample->recordID = recordID;
	sample->delta = delta;
	profiler->sampleIndex = sampleIndex + 1;
}

#endif

txBoolean fxIsProfiling(txMachine* the)
{
#ifdef mxProfile
	return (the->profiler) ? 1 : 0;
#else
	return 0;
#endif
}

void fxStartProfiling(txMachine* the)
{
#ifdef mxProfile
	txProfiler* profiler = the->profiler;
	if (profiler)
		return;	
	if (the->frame)
		fxAbort(the, XS_FATAL_CHECK_EXIT);
	fxCreateProfiler(the);
#endif
}

void fxStopProfiling(txMachine* the)
{
#ifdef mxProfile
	txProfiler* profiler = the->profiler;
	if (!profiler)
		return;	
	if (the->frame)
		fxAbort(the, XS_FATAL_CHECK_EXIT);
	profiler->stop = mxProfileTime();
	fxPrintProfiler(the);
	fxDeleteProfiler(the);
#endif
}

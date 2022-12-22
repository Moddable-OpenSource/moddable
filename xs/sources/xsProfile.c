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


#define _GNU_SOURCE
#include "xsAll.h"
#if mxMacOSX || mxLinux
#include <dlfcn.h>
#endif

#ifdef mxProfile

typedef struct sxProfiler txProfiler;
typedef struct sxProfilerRecord txProfilerRecord;
typedef struct sxProfilerSample txProfilerSample;

struct sxProfiler {
	txU8 when;
	txU8 former;
	txU8 start;
	txU8 stop;
	txU4 interval;
	txU4 recordCount;
	txProfilerRecord** records;
	txU8 sampleCount;
	txU8 sampleIndex;
	txProfilerSample* samples;
	txProfilerRecord* host;
	txProfilerRecord* gc;
};

struct sxProfilerRecord {
	txID recordID;
	txID constructorID;
	txID prototypeID;
	txID functionID;
	txCallback functionAddress;
	txID file;
	txInteger line;
	txInteger hitCount;
	txInteger calleeCount;
	txID* callees;
	txInteger flags;
};

struct sxProfilerSample {
	txID recordID;
	txU4 delta;
};

static txProfilerRecord* fxFrameToProfilerRecord(txMachine* the, txSlot* frame);
static txU8 fxGetTicks();
static void fxInsertProfilerCallee(txMachine* the, txProfilerRecord* record, txID recordID);
static txU8 fxMicrosecondsToTicks(txU8 microseconds);
static void fxPrintID(txMachine* the, FILE* file, txID id);
static void fxPrintProfiler(txMachine* the, void* stream);
static void fxPrintString(txMachine* the, FILE* file, txString theString);
static void fxPushProfilerSample(txMachine* the, txID recordID, txU4 delta);
static txProfilerRecord* fxNewProfilerRecord(txMachine* the, txID recordID);
static txID fxRemoveProfilerCycle(txMachine* the, txProfiler* profiler, txID recordID);
static txU8 fxTicksToMicroseconds(txU8 ticks);

void fxCheckProfiler(txMachine* the, txSlot* frame)
{
	txProfiler* profiler = the->profiler;
	if (!profiler)
		return;
	txU8 when = profiler->when;
	txU8 time = fxGetTicks();
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
		txU4 interval = profiler->interval;
		record->hitCount++;
		fxPushProfilerSample(the, record->recordID, (txU4)(time - profiler->former));
		profiler->former = time;
		profiler->when = time + interval - (time % interval);
	}
}

void fxCreateProfiler(txMachine* the)
{
	txProfiler* profiler = the->profiler = c_malloc(sizeof(txProfiler));
	if (profiler == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	profiler->interval = (txU4)fxMicrosecondsToTicks(1250);
	profiler->former = fxGetTicks();
	profiler->when = profiler->former + profiler->interval;
	profiler->start = profiler->former;
	
	profiler->recordCount = 2;
	profiler->records = (txProfilerRecord**)c_calloc(sizeof(txProfilerRecord*), profiler->recordCount);
	if (profiler->records == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
		
	profiler->sampleCount = 1024;
	profiler->sampleIndex = 0;
	profiler->samples = (txProfilerSample*)c_malloc((size_t)(profiler->sampleCount * sizeof(txProfilerSample)));
	if (profiler->samples == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	profiler->host = fxNewProfilerRecord(the, 0);
	profiler->gc = fxNewProfilerRecord(the, 1);
}

void fxDeleteProfiler(txMachine* the, void* stream)
{
	txProfiler* profiler = the->profiler;
	profiler->stop = profiler->when;
	fxPrintProfiler(the, stream);
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
			txSlot* code = mxFunctionInstanceCode(function);
			txSlot* home = mxFunctionInstanceHome(function);
			txID recordID = home->ID;
			txProfilerRecord* record = C_NULL;
			if (recordID < profiler->recordCount)
				record = profiler->records[recordID];
			if (record)
				return record;
			record = fxNewProfilerRecord(the, recordID);
			record->functionID = code->ID;
			home = home->value.home.object;
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
			if ((code->kind == XS_CODE_KIND) || (code->kind == XS_CODE_X_KIND)) {
				txByte* p = code->value.code.address + 2;
				if (*p == XS_CODE_FILE) {
					txID file;
					txS2 line;
					p++;
					mxDecodeID(p, file);
					p++;
					mxDecode2(p, line);
					record->file = file;
					record->line = line;
				}
				record->functionAddress = C_NULL;
			}
			else
				record->functionAddress = code->value.callback.address;
			return record;
		}
	}
	return C_NULL;
}

txU8 fxGetTicks()
{
#if mxMacOSX
	return clock_gettime_nsec_np(CLOCK_MONOTONIC);
#elif mxWindows
	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	return counter.QuadPart;
#else
	c_timeval tv;
	c_gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000000ULL) + tv.tv_usec;
#endif
}

void fxInsertProfilerCallee(txMachine* the, txProfilerRecord* record, txID recordID)
{
	txInteger count = record->calleeCount;
	if (count == 0) {
		record->callees = c_malloc(sizeof(txID));
		if (record->callees == C_NULL)
			fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
		record->calleeCount = 1;
		record->callees[0] = recordID;
		return;
	}
    txInteger min = 0;
	txInteger max = count;
	txInteger mid;
	txID* callee = record->callees;
	while (min < max) {
		mid = (min + max) >> 1;
		callee = record->callees + mid;
		if (recordID < *callee)
			max = mid;
		else if (recordID > *callee)
			min = mid + 1;
		else
			return;
	}
	if (recordID > *callee)
		mid++;
	record->calleeCount++;
	record->callees = c_realloc(record->callees, record->calleeCount * sizeof(txID));
	if (record->callees == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	callee = record->callees + mid;
	if (mid < count)
		c_memmove(callee + 1, callee, (count - mid) * sizeof(txID));
	*callee = recordID;
}

txU8 fxMicrosecondsToTicks(txU8 microseconds)
{
#if mxMacOSX
	return microseconds * 1000;
#elif mxWindows
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return (frequency.QuadPart * microseconds) / 1000000ULL;
#else
	return microseconds;
#endif
}

txProfilerRecord* fxNewProfilerRecord(txMachine* the, txID recordID)
{
	txProfiler* profiler = the->profiler;
	txU4 recordCount = profiler->recordCount;
	txProfilerRecord* record;
	if (recordID >= recordCount) {
		recordCount = recordID + 1;
		profiler->records = (txProfilerRecord**)c_realloc(profiler->records, recordCount * sizeof(txProfilerRecord*));
		if (profiler->records == C_NULL)
			fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
		c_memset(profiler->records + profiler->recordCount, 0, (recordCount - profiler->recordCount) * sizeof(txProfilerRecord*));
		profiler->recordCount = recordCount;
	}
	record = c_malloc(sizeof(txProfilerRecord));
	if (record == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	record->recordID = recordID;
	record->hitCount = 0;
	record->constructorID = XS_NO_ID;
	record->prototypeID = XS_NO_ID;
	record->functionID = XS_NO_ID;
	record->file = XS_NO_ID;
	record->line = 0;
	record->calleeCount = 0;
	record->callees = C_NULL;
	record->flags = 0;
	profiler->records[recordID] = record;
	return record;
}

void fxPrintID(txMachine* the, FILE* file, txID id)
{
	txSlot* key = fxGetKey(the, id);
	if (key) {
		if ((key->kind == XS_KEY_KIND) || (key->kind == XS_KEY_X_KIND)) {
			fxPrintString(the, file,  key->value.key.string);
			return;
		}
		if ((key->kind == XS_STRING_KIND) || (key->kind == XS_STRING_X_KIND)) {
			fprintf(file, "[");
			fxPrintString(the, file, key->value.string);
			fprintf(file, "]");
			return;
		}
	}
	fprintf(file, "?");
}

void fxPrintProfiler(txMachine* the, void* stream)
{
	// https://chromedevtools.github.io/devtools-protocol/tot/Profiler/#type-Profile
	txProfiler* profiler = the->profiler;
	FILE* file = stream;
    char buffer[22];
    char name[36];
    
	fxRemoveProfilerCycle(the, profiler, 0);
    
    if (!file) {
    	time_t timer;
    	struct tm* tm_info;
		timer = time(NULL);
		tm_info = localtime(&timer);
		strftime(buffer, 22, "XS-%y-%m-%d-%H-%M-%S-", tm_info);
		snprintf(name, sizeof(name), "%s%03llu.cpuprofile", buffer, (profiler->stop / 1000) % 1000);
		file = fopen(name, "w");
	}
	
	fprintf(file, "{\"nodes\":[");
	txU4 recordIndex = 0;
	while (recordIndex < profiler->recordCount) {
		txProfilerRecord* record = profiler->records[recordIndex];
		if (record) {
			if (recordIndex > 0)
				fprintf(file, ",");
			fprintf(file, "{\"id\":%d,\"callFrame\":{\"functionName\":\"", record->recordID);
			if (recordIndex == 0)
				fprintf(file, "(host)");
			else if (recordIndex == 1)
				fprintf(file, "(gc)");
			else if (record->functionID != XS_NO_ID) {
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
			else if (record->functionAddress) {
#if mxMacOSX || mxLinux
				Dl_info info;
				if (dladdr(record->functionAddress, &info) && info.dli_sname)
					fprintf(file, "@%s",info.dli_sname );
				else
#endif
					fprintf(file, "@anonymous-%d", record->recordID);
			}
			else
				fprintf(file, "(anonymous-%d)", record->recordID);

			fprintf(file, "\",\"scriptId\":\"0\",\"url\":\"");
			if (record->file != XS_NO_ID)
				fxPrintID(the, file, record->file);
			fprintf(file, "\",\"lineNumber\":%d,\"columnNumber\":-1},\"hitCount\":%d,\"children\":[", record->line - 1, record->hitCount);
			txInteger calleeIndex = 0;
			txBoolean comma = 0;
			while (calleeIndex < record->calleeCount) {
				txID recordID = record->callees[calleeIndex];
				if (recordID != XS_NO_ID) {
					if (comma)
						fprintf(file, ",");
					fprintf(file, "%d", recordID);
					comma = 1;
				}
				calleeIndex++;
			}
			fprintf(file, "]}");
		}
		recordIndex++;
	}
	fprintf(file, "],\"startTime\":%llu,\"endTime\":%llu,\"samples\":[", fxTicksToMicroseconds(profiler->start), fxTicksToMicroseconds(profiler->when));
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
			fprintf(file, "%llu", fxTicksToMicroseconds(sample->delta));
			sampleIndex++;
		}
	}
	fprintf(file, "]}");
	fclose(file);
}

void fxPrintString(txMachine* the, FILE* file, txString theString)
{
	char buffer[16];
	for (;;) {
		txInteger character;
		char *p;
		theString = mxStringByteDecode(theString, &character);
		if (character == C_EOF)
			break;
		p = buffer;
		if ((character < 32) || ((0xD800 <= character) && (character <= 0xDFFF))) {
			*p++ = '\\'; 
			*p++ = 'u'; 
			p = fxStringifyUnicodeEscape(p, character, '\\');
		}
		else if (character == '"') {
			*p++ = '\\';
			*p++ = '"';
		}
		else if (character == '\\') {
			*p++ = '\\';
			*p++ = '\\';
		}
		else
			p = mxStringByteEncode(p, character);
		*p = 0;
		fprintf(file, "%s", buffer);
	}
}

void fxPushProfilerSample(txMachine* the, txID recordID, txU4 delta)
{
	txProfiler* profiler = the->profiler;
	txU8 sampleCount = profiler->sampleCount;
	txU8 sampleIndex = profiler->sampleIndex;
	txProfilerSample* sample;
	if (sampleIndex == sampleCount) {
		sampleCount += 1024;
		profiler->samples = (txProfilerSample*)c_realloc(profiler->samples, (size_t)(sampleCount * sizeof(txProfilerSample)));
		if (profiler->samples == C_NULL)
			fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
		profiler->sampleCount = sampleCount;
	}
	sample = profiler->samples + sampleIndex;
	sample->recordID = recordID;
	sample->delta = delta;
	profiler->sampleIndex = sampleIndex + 1;
}

txID fxRemoveProfilerCycle(txMachine* the, txProfiler* profiler, txID recordID)
{
	txProfilerRecord* record = profiler->records[recordID];
	if (record->flags & 1)
		return XS_NO_ID;
	if (record->flags & 2)
		return recordID;
	record->flags |= 3;
	txInteger calleeIndex = 0;
	while (calleeIndex < record->calleeCount) {
		txID calleeID = record->callees[calleeIndex];
		if (calleeID)
			record->callees[calleeIndex] = fxRemoveProfilerCycle(the, profiler, calleeID);
		calleeIndex++;
	}	
	record->flags &= ~1;
	return recordID;
}

void fxResumeProfiler(txMachine* the)
{
	txProfiler* profiler = the->profiler;
	if (!profiler)
		return;
	txU8 delta = fxGetTicks() - profiler->stop;
	profiler->when += delta;
	profiler->former += delta;
	profiler->start += delta;
}

void fxSuspendProfiler(txMachine* the)
{
	txProfiler* profiler = the->profiler;
	if (!profiler)
		return;
	profiler->stop = fxGetTicks();
}

txU8 fxTicksToMicroseconds(txU8 ticks)
{
#if mxMacOSX
	return ticks / 1000;
#elif mxWindows
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	return (1000000ULL * ticks) / frequency.QuadPart;
#else
	return ticks;
#endif
}



#endif /* mxProfile */

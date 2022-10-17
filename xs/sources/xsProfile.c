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
typedef struct sxProfilerNode txProfilerNode;
typedef struct sxProfilerSample txProfilerSample;

struct sxProfiler {
	txU8 delta;
	txU8 time;
	txU8 start;
	txU8 stop;
	txU4 nodeCount;
	txU4 nodeIndex;
	txProfilerNode** nodes;
	txU4 sampleCount;
	txU4 sampleIndex;
	txProfilerSample* samples;
	txProfilerNode* root;
};

struct sxProfilerNode {
	txU4 index;
	txID id;
	txID file;
	txInteger line;
	txInteger ticks;
	txInteger childCount;
	txU4* children;
};

struct sxProfilerSample {
	txU4 index;
	txU4 delta;
};

static txProfilerNode* fxFrameToProfilerNode(txMachine* the, txSlot* frame);
static void fxInsertProfilerNodeChild(txMachine* the, txProfilerNode* node, txU4 index);
static txProfilerNode* fxNewProfilerNode(txMachine* the);
static void fxNewProfilerSample(txMachine* the, txU4 index, txU4 delta);

void fxCreateProfiler(txMachine* the)
{
	txProfiler* profiler = the->profiler = c_malloc(sizeof(txProfiler));
	if (profiler == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	profiler->delta = 1000;
	profiler->time = mxProfileTime() + profiler->delta;
	profiler->start = profiler->time;
	profiler->nodeCount = 1024;
	profiler->nodeIndex = 0;
	profiler->nodes = (txProfilerNode**)c_malloc(1024 * sizeof(txProfilerNode*));
	if (profiler->nodes == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	profiler->sampleCount = 1024;
	profiler->sampleIndex = 0;
	profiler->samples = (txProfilerSample*)c_malloc(1024 * sizeof(txProfilerSample));
	if (profiler->samples == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	profiler->root = fxNewProfilerNode(the);
}

void fxDeleteProfiler(txMachine* the)
{
	txProfiler* profiler = the->profiler;
	FILE* file = stderr;
	profiler->stop = mxProfileTime();
	
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
	txU4 nodeIndex = 0;
	while (nodeIndex < profiler->nodeIndex) {
		txProfilerNode* node = profiler->nodes[nodeIndex];
		if (nodeIndex > 0)
			fprintf(file, ",");
		fprintf(file, "{\"id\":%d,\"callFrame\":{\"functionName\":\"", node->index);
		if (node->id != XS_NO_ID) {
			txSlot* key = fxGetKey(the, node->id);
			if (key) {
				if ((key->kind == XS_KEY_KIND) || (key->kind == XS_KEY_X_KIND)) {
					fprintf(file, "%s", key->value.key.string);
				}
				else if ((key->kind == XS_STRING_KIND) || (key->kind == XS_STRING_X_KIND)) {
					fprintf(file, "[%s]", key->value.string);
				}
			}
		}
		else if (nodeIndex == 0)
			fprintf(file, "(root)");
		else
			fprintf(file, "(anonymous)");
		fprintf(file, "\",\"scriptId\":\"0\",\"url\":\"");
		if (node->file != XS_NO_ID) {
			txSlot* key = fxGetKey(the, node->file);
			if (key) {
				fprintf(file, "%s", key->value.key.string);
			}
		}
		fprintf(file, "\",\"lineNumber\":%d,\"columnNumber\":-1},\"hitCount\":%d,\"children\":[", node->line - 1, node->ticks);
		txInteger childIndex = 0;
		while (childIndex < node->childCount) {
			if (childIndex > 0)
				fprintf(file, ",");
			fprintf(file, "%d", node->children[childIndex]);
			childIndex++;
		}
		fprintf(file, "]}");
		nodeIndex++;
	}
	fprintf(file, "],\"startTime\":%lld,\"endTime\":%lld,\"samples\":[", profiler->start, profiler->stop);
	{
		txU4 sampleIndex = 0;
		while (sampleIndex < profiler->sampleIndex) {
			txProfilerSample* sample = profiler->samples + sampleIndex;
			if (sampleIndex > 0)
				fprintf(file, ",");
			fprintf(file, "%d", sample->index);
			sampleIndex++;
		}
	}
	fprintf(file, "],\"timeDeltas\":[");
	{
		txU4 sampleIndex = 0;
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
	
	c_free(profiler->samples);
	c_free(profiler->nodes);
	c_free(the->profiler);
}

void fxEnterGC(txMachine* the)
{
}

void fxLeaveGC(txMachine* the)
{
}

void fxProfilerLine(txMachine* the, txSlot* frame, txID file, txInteger line)
{
	txProfiler* profiler = the->profiler;
	txU8 time = mxProfileTime();
	if (profiler->time < time) {
		txProfilerNode* child = C_NULL;
		txProfilerNode* node = fxFrameToProfilerNode(the, frame);
		if (node) {
			child = node;
			frame = frame->next;
			while (frame) {
				txProfilerNode* parent = fxFrameToProfilerNode(the, frame);
				if (parent) {
					fxInsertProfilerNodeChild(the, parent, child->index);
					child = parent;
				}
				frame = frame->next;
			}
		}
		if (child)
			fxInsertProfilerNodeChild(the, profiler->root, child->index);
		else
			node = profiler->root;
		node->ticks++;
		fxNewProfilerSample(the, node->index, profiler->delta + time - profiler->time);
		profiler->time = time + profiler->delta;
	}
}

txProfilerNode* fxFrameToProfilerNode(txMachine* the, txSlot* frame)
{
	txSlot* function = frame + 3;
	if (function->kind == XS_REFERENCE_KIND) {
		function = function->value.reference;
		if (mxIsFunction(function)) {
			txSlot* profile = mxFunctionInstanceProfile(function);
			txProfilerNode* node = profile->value.profile.node;
			if (node)
				return node;
			node = fxNewProfilerNode(the);
			node->id = mxFunctionInstanceCode(function)->ID;
			node->file = profile->value.profile.file;
			node->line = profile->value.profile.line;
			profile->value.profile.node = node;
			return node;
		}
	}
	return C_NULL;
}

void fxInsertProfilerNodeChild(txMachine* the, txProfilerNode* node, txU4 index)
{
	txInteger count = node->childCount;
	if (count == 0) {
		node->children = c_malloc(sizeof(txU4));
		if (node->children == C_NULL)
			fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
		node->childCount = 1;
		node->children[0] = index;
		return;
	}
	txInteger cmp = -1;
    txInteger min = 0;
	txInteger max = count;
	txInteger mid;
	txU4* child;
	while (min < max) {
		mid = (min + max) >> 1;
		child = node->children + mid;
		cmp = index - *child;
		if (cmp < 0)
			max = mid;
		else if (cmp > 0)
			min = mid + 1;
		else
			return;
	}
	if (cmp > 0)
		mid++;
	node->childCount++;
	node->children = c_realloc(node->children, node->childCount * sizeof(txU4));
	if (node->children == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	child = node->children + mid;
	if (mid < count)
		c_memmove(child + 1, child, (count - mid) * sizeof(txU4));
	*child = index;
}

txProfilerNode* fxNewProfilerNode(txMachine* the)
{
	txProfiler* profiler = the->profiler;
	txInteger nodeCount = profiler->nodeCount;
	txInteger nodeIndex = profiler->nodeIndex;
	txProfilerNode* node;
	if (nodeIndex == nodeCount) {
		nodeCount += 1024;
		profiler->nodes = (txProfilerNode**)c_realloc(profiler->nodes, nodeCount * sizeof(txProfilerNode*));
		if (profiler->nodes == C_NULL)
			fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
		profiler->nodeCount = nodeCount;
	}
	node = c_malloc(sizeof(txProfilerNode));
	if (node == C_NULL)
		fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
	node->index = nodeIndex;
	node->ticks = 0;
	node->id = XS_NO_ID;
	node->file = XS_NO_ID;
	node->line = 0;
	node->childCount = 0;
	node->children = C_NULL;
	profiler->nodes[nodeIndex] = node;
	profiler->nodeIndex = nodeIndex + 1;
	return node;
}

void fxNewProfilerSample(txMachine* the, txU4 index, txU4 delta)
{
	txProfiler* profiler = the->profiler;
	txInteger sampleCount = profiler->sampleCount;
	txInteger sampleIndex = profiler->sampleIndex;
	txProfilerSample* sample;
	if (sampleIndex == sampleCount) {
		sampleCount += 1024;
		profiler->samples = (txProfilerSample*)c_realloc(profiler->samples, sampleCount * sizeof(txProfilerSample));
		if (profiler->samples == C_NULL)
			fxAbort(the, XS_NOT_ENOUGH_MEMORY_EXIT);
		profiler->sampleCount = sampleCount;
	}
	sample = profiler->samples + sampleIndex;
	sample->index = index;
	sample->delta = delta;
	profiler->sampleIndex = sampleIndex + 1;
}

#endif

txBoolean fxIsProfiling(txMachine* the)
{
#ifdef mxProfile
	return 1;
#else
	return 0;
#endif
}

void fxStartProfiling(txMachine* the)
{
}

void fxStopProfiling(txMachine* the)
{
}

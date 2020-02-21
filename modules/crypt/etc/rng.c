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

#include "xsPlatform.h"
#include "xsmc.h"
#include <stdint.h>
#include <stddef.h>

static uint8_t rng_state[256];
static uint8_t rng_inited = 0;

#define DEFAULT_SEED_SIZE	16

static void
rng_init(uint8_t *seed, size_t seedsize)
{
	int i;
	uint8_t j, t;
	uint8_t seedbuf[DEFAULT_SEED_SIZE];

	if ((NULL == seed) || (0 == seedsize)) {
		for (i = 0; i < DEFAULT_SEED_SIZE; i++)
			seedbuf[i] = ((uint64_t)c_rand() * 256) / C_RAND_MAX;
		seed = seedbuf;
		seedsize = DEFAULT_SEED_SIZE;
	}

	for (i = 0; i < 256; i++)
		rng_state[i] = i;

	for (i = j = 0; i < 256; i++) {
		j += rng_state[i] + seed[i % seedsize];
		t = rng_state[i];
		rng_state[i] = rng_state[j];
		rng_state[j] = t;
	}

	rng_inited = 1;
}

void
xs_rng_get(xsMachine *the)
{
	size_t n = xsmcToInteger(xsArg(0));
	uint8_t *bp, *bufend;
	uint8_t i, j, t;

	if (!rng_inited)
		rng_init(NULL, 0);

	xsmcSetArrayBuffer(xsResult, NULL, n);
	bp = xsmcToArrayBuffer(xsResult);
	for (i = j = 0, bufend = bp + n; bp < bufend;) {
		++i;
		j += rng_state[i];
		t = rng_state[i];
		rng_state[i] = rng_state[j];
		rng_state[j] = t;
		t = rng_state[i] + rng_state[j];
		*bp++ = rng_state[t];
	}
}

void
xs_rng_init(xsMachine *the)
{
	uint8_t *seed = NULL;
	size_t seedsize = 0;

	if (xsmcArgc > 0 && xsmcTest(xsArg(0))) {
		seed = xsmcToArrayBuffer(xsArg(0));
		seedsize = xsmcGetArrayBufferLength(xsArg(0));
	}
	rng_init(seed, seedsize);
}

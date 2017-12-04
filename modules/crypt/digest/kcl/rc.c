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

#include "rc.h"

void
rc4_init(rc4_state_t *rc4, const uint8_t *key, uint32_t keysize)
{
	int i;
	uint8_t j, t;
	uint8_t *s;

	for (i = 0, s = rc4->state; i < 256; i++)
		*s++ = i;
	for (i = j = 0, s = rc4->state; i < 256; i++) {
		j += s[i] + key[i % keysize];
		t = s[i];
		s[i] = s[j];
		s[j] = t;
	}
	rc4->i = rc4->j = 0;
}

void
rc4_process(const uint8_t *indata, uint8_t *outdata, unsigned int nbytes, rc4_state_t *rc4)
{
	const uint8_t *endp = indata + nbytes;
	uint8_t i, j, t;
	uint8_t *s = rc4->state;

	while (indata < endp) {
		i = ++rc4->i;
		j = rc4->j += s[i];
		t = s[i];
		s[i] = s[j];
		s[j] = t;
		t = s[i] + s[j];
		*outdata++ = *indata++ ^ s[t];
	}
}

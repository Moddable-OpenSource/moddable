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

#define COMPILE_WORK_LINK_COUNT 1024
#define LINK_SIZE 2
#define MATCH_LIMIT 10000000
#define MATCH_LIMIT_RECURSION MATCH_LIMIT
#define MAX_NAME_COUNT 10000
#define MAX_NAME_SIZE 32
#define NEWLINE 10
#define PARENS_NEST_LIMIT 250
#define PCREGREP_BUFSIZE 20480
#define PCRE_STATIC 1
#define POSIX_MALLOC_THRESHOLD 10

#define STDC_HEADERS 1
#define SUPPORT_PCRE8 1
#define SUPPORT_UCP 1
#define SUPPORT_UNICODE 1
#define SUPPORT_UTF 1
#define SUPPORT_XS 1

#include "pcre_byte_order.c"
#include "pcre_chartables.c"
#include "pcre_compile.c"
#undef NLBLOCK
#undef PSSTART
#undef PSEND
#include "pcre_exec.c"
#include "pcre_fullinfo.c"
#include "pcre_globals.c"
#include "pcre_newline.c"
#include "pcre_ord2utf8.c"
#include "pcre_tables.c"
#include "pcre_ucd.c"
#include "pcre_valid_utf8.c"
#include "pcre_xclass.c"

void *pcre_c_malloc(size_t size)
{
	void* p = c_malloc(size);
	//fprintf(stderr, "pcre_c_malloc %ld %p\n", size, p);
	return p;
}

void pcre_c_free(void *p)
{
	//fprintf(stderr, "pcre_c_free %p\n", p);
	c_free(p);
}

int pcre_c_stack_guard(void)
{
#ifdef c_stackspace
	return c_stackspace() < 650;
#else
	return 0;
#endif
}

txBoolean fxCompileRegExp(void* the, txString pattern, txString modifier, void** code, void** data, txUnsigned* flags, txString messageBuffer, txInteger messageSize)
{
	char c;
	void* _code;
	void* _data;
	txUnsigned _flags = 0;
	txInteger options;
	txString message;
	txInteger offset;
	pcre_malloc = pcre_c_malloc;
	pcre_free = pcre_c_free;
	pcre_stack_guard = pcre_c_stack_guard;
	while ((c = *modifier++)) {
		if ((c == 'g') && !(_flags & XS_REGEXP_G))
			_flags |= XS_REGEXP_G;
		else if ((c == 'i') && !(_flags & XS_REGEXP_I))
			_flags |= XS_REGEXP_I;
		else if ((c == 'm') && !(_flags & XS_REGEXP_M))
			_flags |= XS_REGEXP_M;
		else if ((c == 'u') && !(_flags & XS_REGEXP_U))
			_flags |= XS_REGEXP_U;
		else if ((c == 'y') && !(_flags & XS_REGEXP_Y))
			_flags |= XS_REGEXP_Y;
		else
			break;
	}
	if (c) {
		if (messageBuffer)
			c_strcpy(messageBuffer, "invalid flags");
		return 0;
	}	
	options = PCRE_UTF8; 
	options |= PCRE_JAVASCRIPT_COMPAT;
	if (_flags & XS_REGEXP_I)
		options |= PCRE_CASELESS;
	if (_flags & XS_REGEXP_M)
		options |= PCRE_MULTILINE;
	if (_flags & XS_REGEXP_U)
		options |= PCRE_UCP;
	options |= PCRE_NEWLINE_ANY;
	_code = pcre_compile(pattern, options, (const char **)(void*)&message, (int*)(void*)&offset, NULL); /* use default character tables */
	if (!_code) {
		if (messageBuffer)
			c_strncpy(messageBuffer, message, messageSize - 1);
		return 0;
	}
	if (data) {
		txInteger count;
		txInteger limit;
		pcre_fullinfo(_code, NULL, PCRE_INFO_CAPTURECOUNT, &count);
		count++;
		pcre_fullinfo(_code, NULL, PCRE_INFO_BACKREFMAX, &limit);
		if (count < limit)
			count = limit;
		count *= 3;
		_flags |= count & XS_REGEXP_COUNT_MASK;
	#ifdef mxRun
		if (the)
			_data = fxNewChunk(the, count * sizeof(txInteger));
		else
	#endif
			_data = pcre_malloc(count * sizeof(txInteger));
		if (!_data) {
			pcre_free(_code);
			if (messageBuffer)
				c_strcpy(messageBuffer, "not enough memory");
			return 0;
		}
        *data = _data;
	}
	if (code) {
	#ifdef mxRun
		if (the) {
			size_t size;
			void* chunk;
			pcre_fullinfo(_code, NULL, PCRE_INFO_SIZE, &size);
			chunk = fxNewChunk(the, size);
			c_memcpy(chunk, _code, size);
			pcre_free(_code);
			*code = chunk;
		}
		else
	#endif
			*code = _code;
	}
	else
		pcre_free(_code);
	if (flags)
		*flags = _flags;
	return 1;
}

void fxDeleteRegExp(void* the, void* code, void* data)
{
	if (code)
		pcre_free(code);
	if (data)
		pcre_free(data);
}

txInteger fxMatchRegExp(void* the, void* code, void* data, txUnsigned flags, txString subject, txInteger offset, txInteger** offsets, txInteger* limit)
{
	txInteger count;
	pcre_malloc = pcre_c_malloc;
	pcre_free = pcre_c_free;
	count = pcre_exec((const pcre *)code, NULL, subject, c_strlen(subject), offset, (flags & XS_REGEXP_Y) ? PCRE_ANCHORED : 0, (int*)data, flags & XS_REGEXP_COUNT_MASK);
	if (count > 0) {
		if (offsets)
			*offsets = (txInteger*)data;
		if (limit) {
			int _limit = 0;
			pcre_fullinfo((const pcre *)code, NULL, PCRE_INFO_CAPTURECOUNT, &_limit);
			_limit++;
			*limit = _limit;
		}
	}
	return count;
}



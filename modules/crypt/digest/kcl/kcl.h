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

#ifndef __KCL_H__
#define __KCL_H__

#include "xsPlatform.h"
#include <stddef.h>	/* for size_t */

typedef enum {
	KCL_ERR_NONE = 0,
	KCL_ERR_NAN = 9000,
	KCL_ERR_OUT_OF_RANGE,
	KCL_ERR_BAD_ARG,
	KCL_ERR_TYPE,
	KCL_ERR_NOMEM,
	KCL_ERR_BAD_OPERATION,
	KCL_ERR_UNKNOWN,
} kcl_err_t;

typedef struct {
	void (*f)(kcl_err_t, void *);
	void *closure;
} kcl_error_callback_t;

//extern void *kcl_malloc(size_t sz);
//extern void kcl_free(void *p);
#define kcl_malloc c_malloc
#define kcl_free c_free

#endif /* __KCL_H__ */

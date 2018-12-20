/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "xsPlatform.h"
#include "xsAll.h"
#include "xs.h"
#include "mc.xs.h"
extern txPreparation* xsPreparation();

#if mxWindows
	#include <direct.h>
	#include <errno.h>
	#include <process.h>
	#define mxSeparator '\\'
	#define PATH_MAX 1024
#else
	#include <dirent.h>
	#include <sys/stat.h>
	#include <unistd.h>
	#define mxSeparator '/'
#endif

#ifdef mxDebug
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (fxThrowMessage(the,(char *)__FILE__,__LINE__,XS_UNKNOWN_ERROR,"%s",strerror(errno)), 1)))
#else
	#define xsElseThrow(_ASSERTION) \
		((void)((_ASSERTION) || (fxThrowMessage(the,NULL,0,XS_UNKNOWN_ERROR,"%s",strerror(errno)), 1)))
#endif

* Copyright (c) 2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

#include "stdint.h"

extern int32_t add32_t(int32_t arg0, int32_t arg1);
extern int64_t add64_t(int64_t arg0, int64_t arg1);

int32_t add32_t(int32_t arg0, int32_t arg1)
{
	return arg0 + arg1;
}

int64_t add64_t(int64_t arg0, int64_t arg1)
{
	return arg0 + arg1;
}

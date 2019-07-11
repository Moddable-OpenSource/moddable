/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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

#include "xsmc.h"
#if ESP32 || __ets__
	#include "xsesp.h"
#elif defined(qca4020)
	#include "xsqca4020.h"
	#include "xsPlatform.h"
#endif

void do_restart(xsMachine *the)
{
#if ESP32 
	esp_restart();
#elif __ets__
	system_restart();
#elif defined(qca4020)
	qca4020_reset();
#endif
}

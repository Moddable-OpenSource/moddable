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
 */

#define __XS6PLATFORMMINIMAL__
#include "xs.h"
#include "xsgecko.h"

#include "xsPlatform.h"
#include "modInstrumentation.h"

xsMachine *gThe;        // the one XS6 virtual machine running

xsCallback xsHostModuleAt(xsIndex i)
{
    return NULL;
}

#define _OLD_STYLE 0

#if _OLD_STYLE
static const char gSetup[] = "setup";
static const char gRequire[] = "require";
static const char gWeak[] = "weak";
static const char gMain[] = "main";
#else
extern void mc_setup(xsMachine *the);
#endif

void xs_setup() {
    const char *module;
    gThe = ESP_cloneMachine(0, 0, 0, 0);

#if _OLD_STYLE
    xsBeginHost(gThe);
    xsResult = xsString(gSetup);
    if (XS_NO_ID != fxFindModule(the, XS_NO_ID, &xsResult))
        module = gSetup;
    else
        module = gMain;

    xsResult = xsGet(xsGlobal, xsID(gRequire));
    xsResult = xsCall1(xsResult, xsID(gWeak), xsString(module));
    if (xsTest(xsResult) && xsIsInstanceOf(xsResult, xsFunctionPrototype))
        xsCallFunction0(xsResult, xsGlobal);
    xsEndHost(gThe);
#else
	mc_setup(gThe);
#endif
}

void xs_loop(void)
{
    if (!gThe)
        return;

#if mxDebug
    if (ESP_isReadable()) {
        if (triggerDebugCommand(gThe)) {
            if (modTimersNextScript() > 500) {      // if a script is not likely to fire within half a second, break immediately
                xsBeginHost(gThe);
                xsDebugger();
                xsEndHost(gThe);
            }
        }
    }
#endif

    modTimersExecute();
	
	modMessageService();

    if (modRunPromiseJobs(gThe))
    	return;

    int delayMS = modTimersNext();

    if (delayMS)
    	gecko_delay(delayMS);
}



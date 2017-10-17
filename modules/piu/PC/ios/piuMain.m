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

#include "piuPC.h"

int main(int argc, char *argv[])
{
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass([UIPiuAppDelegate class]));
    }
}

void fxAbort(xsMachine *the)
{
}

void PiuApplication_createMenus(xsMachine *the)
{
}

void PiuApplication_get_title(xsMachine *the)
{
}

void PiuApplication_set_title(xsMachine *the)
{
}

void PiuApplication_gotoFront(xsMachine *the)
{
}

void PiuApplication_purge(xsMachine* the)
{
	xsCollectGarbage();
}

void PiuApplication_quit(xsMachine *the)
{
}

void PiuApplication_updateMenus(xsMachine *the)
{
}

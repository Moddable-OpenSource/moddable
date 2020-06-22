/*
 * Copyright (c) 2016-2020  Moddable Tech, Inc.
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

#include "xsPlatform.h"
#include "xsmc.h"
#include "mc.xs.h"
#include "modBLE.h"

void xs_gap_whitelist_add(xsMachine *the)
{
	xsUnknownError("whitelist unsupported");
}

void xs_gap_whitelist_remove(xsMachine *the)
{
	xsUnknownError("whitelist unsupported");
}

void xs_gap_whitelist_clear(xsMachine *the)
{
	xsUnknownError("whitelist unsupported");
}

modBLEWhitelistAddress modBLEGetWhitelist(void)
{
	// unused on gecko
	return NULL;
}

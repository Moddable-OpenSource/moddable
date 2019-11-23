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


#include "xsmc.h"
#include "xsHost.h"
#include "user_interface.h"

void xs_esp_reset(xsMachine *the)
{
	system_restore();
	wifi_station_set_auto_connect(1);
}

void xs_esp_phy_get(xsMachine *the)
{
	switch (wifi_get_phy_mode()) {
		case PHY_MODE_11B:	xsmcSetString(xsResult, "b"); break;
		case PHY_MODE_11G:	xsmcSetString(xsResult, "g"); break;
		case PHY_MODE_11N:	xsmcSetString(xsResult, "n"); break;
		default: xsmcSetString(xsResult, "(unknown)"); break;
	}
}

void xs_esp_phy_set(xsMachine *the)
{
	const char *phy = xsmcToString(xsArg(0));
	if (0 == c_strcmp(phy, "b"))
		wifi_set_phy_mode(PHY_MODE_11B);
	else if (0 == c_strcmp(phy, "g"))
		wifi_set_phy_mode(PHY_MODE_11G);
	else if (0 == c_strcmp(phy, "n"))
		wifi_set_phy_mode(PHY_MODE_11N);
	else
		xsUnknownError("invalid mode");
}

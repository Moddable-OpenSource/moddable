/*
 * Copyright (c) 2026  Moddable Tech, Inc.
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
#include "flash-esp.h"
#include "upgrade.h"

extern void system_upgrade_flag_set(uint8 flag);		// from SDK user_interface.h, which conflicts with lwIP headers

void xs_update_esp_partition(xsMachine *the)
{
	uint8_t writable;

	if (kESPFlashPartitionNextOTA != getESPFlashPartition(the, &xsArg(0), &writable))
		xsUnknownError("not nextota partition");
	if (!writable)
		xsUnknownError("read-only partition");
}

void xs_update_esp_commit(xsMachine *the)
{
	system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
}

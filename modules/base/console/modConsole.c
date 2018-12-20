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


#include "xsPlatform.h"
#include "xsmc.h"

void xs_console_destructor(void)
{
}

void xs_console_receive(xsMachine *the)
{
	int c = ESP_getc();

	if (-1 == c)
		return;

	xsmcSetInteger(xsResult, c);
}

void xs_console_write(xsMachine *the)
{
	int argc = xsmcArgc, i;

	for (i = 0; i < argc; i++) {
		char *str = xsmcToString(xsArg(i));

		do {
			uint8_t c = c_read8(str);
			if (!c) break;

			ESP_putc(c);

			str++;
		} while (true);
	}
}


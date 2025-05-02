/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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
#include "xsHost.h"
#include "xsHosts.h"

void setupDebugger(void) { }
void modLog_transmit(const char *msg) { (void)msg; }
void ESP_put(uint8_t *c, int count) { (void)c, (void)count; }
void ESP_putc(int c) { (void)c; }
int ESP_getc(void) { return -1; }
uint8_t ESP_isReadable() { return 0; }
uint8_t ESP_setBaud(int baud) { return -1; }


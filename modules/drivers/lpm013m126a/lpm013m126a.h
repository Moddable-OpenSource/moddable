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

#define NO_UPDATE_BLINK 0x00	// 00000000
#define BLINK_BLACK 0x10		// 00010000
#define BLINK_WHITE 0x18		// 00011000
#define BLINK_INVERSION 0x14	// 00010100
#define ALL_CLEAR_COMH 0x60			// 01100000
#define ALL_CLEAR_COML 0x20			// 01100000
#define DATA_UPDATE_3BIT_COMH 0xC0	// 11000000
#define DATA_UPDATE_3BIT_COML 0x80	// 10000000
#define DATA_UPDATE_1BIT 0x88	// 10001000
#define DATA_UPDATE_4BIT 0x90	// 10010000
#define NO_UPDATE_COML 0xA0			// 10100000
#define NO_UPDATE_COMH 0xE0			// 11100000

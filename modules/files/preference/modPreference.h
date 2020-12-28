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

#ifndef __mod_preference__
#define __mod_preference__

#ifdef  __cplusplus
extern "C" {
#endif

enum {
	kPrefsTypeBoolean = 1,
	kPrefsTypeInteger = 2,
	kPrefsTypeString = 3,
	kPrefsTypeBuffer = 4,
};

extern uint8_t modPreferenceSet(char *domain, char *key, uint8_t type, uint8_t *value, uint16_t byteCount);

extern uint8_t modPreferenceGet(char *domain, char *key, uint8_t *type, uint8_t *value, uint16_t byteCountIn, uint16_t *byteCountOut);


#ifdef  __cplusplus
}
#endif

#endif

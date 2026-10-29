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

/*
	Poly1305 one-time authenticator (RFC 8439 section 2.5), 26-bit limbs
*/

#ifndef __POLY1305_H__
#define __POLY1305_H__

#include <stdint.h>

#define POLY1305_KEYSIZE	32
#define POLY1305_TAGSIZE	16

typedef struct {
	uint32_t r[5];
	uint32_t h[5];
	uint32_t pad[4];
	uint8_t buffer[16];
	uint8_t leftover;
} poly1305_t;

extern void poly1305_init(poly1305_t *st, const uint8_t key[POLY1305_KEYSIZE]);
extern void poly1305_update(poly1305_t *st, const uint8_t *m, uint32_t bytes);
extern void poly1305_finish(poly1305_t *st, uint8_t tag[POLY1305_TAGSIZE]);		// clears the state

#endif

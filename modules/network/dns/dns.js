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

const DNS = {
	RR: {
		A: 1,
		PTR: 12,
		TXT: 16,
		AAAA: 28,
		SRV: 33,
		OPT: 41,
		NSEC: 47,
		ANY: 255,
	},
	OPCODE: {
		QUERY: 0,
		UPDATE: 5,
	},
	CLASS: {
		IN: 1,
		NONE: 254,
		ANY: 255,
	},
	SECTION: {
		QUESTION: 0,
		ANSWER: 1,
		AUTHORITATIVE: 2,
		ADDITIONAL: 3,

		ZONE: 0,
		PREREQUISITE: 1,
		UPDATE: 2,
	}
};
Object.freeze(DNS, true);

export default DNS;

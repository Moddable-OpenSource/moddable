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

export default class Vibes {
	static cancel() { return native("xs_vibes_cancel").call(this) };
	static shortPulse() { return native("xs_vibes_shortPulse").call(this) };
	static longPulse() { return native("xs_vibes_longPulse").call(this) };
	static doublePulse() { return native("xs_vibes_doublePulse").call(this) };
	static pattern() { return native("xs_vibes_pattern").call(this) };
}

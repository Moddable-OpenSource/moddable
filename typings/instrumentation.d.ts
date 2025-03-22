/*
 * Copyright (c) 2019-2020 Bradley Farias
 *
 *   This file is part of the Moddable SDK Tools.
 *
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

declare module 'instrumentation' {
	type InstrumentationKeys =
		| 'Pixels Drawn'
		| 'Frames Drawn'
		| 'Network Bytes Read'
		| 'Network Bytes Written'
		| 'Network Sockets'
		| 'Timers'
		| 'Files'
		| 'Poco Display List Used'
		| 'Piu Command List Used'
		| 'Turns'
		| 'CPU 0'
		| 'CPU 1'
		| 'System Free Memory'
		| 'XS Slot Heap Used'
		| 'XS Chunk Heap Used'
		| 'XS Keys Used'
		| 'XS Garbage Collection Count'
		| 'XS Modules Loaded'
		| 'XS Stack Used'
		| 'XS Promises Settled';

	var Instrumentation: {
		get: (what: number | undefined) => number | undefined;
		map: (name: InstrumentationKeys) => number | undefined;
		name: (what: number | undefined) => string | undefined;
	};
	export { Instrumentation as default };
}

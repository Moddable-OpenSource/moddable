/*
* Copyright (c) 2026 Moddable Tech, Inc.
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

// the Pebble touch screen, as an ECMA-419 Touch sensor (build/devices/pebble/modules/touch)

declare module "embedded:sensor/Touch/pebble" {
	interface TouchOptions {
		/** invoked when the touch state changes: call sample() for the touch in progress */
		onSample?: (this: Touch) => void;
		target?: any;
	}

	interface TouchPoint {
		x: number;
		y: number;
		id: number;
	}

	class Touch implements Disposable {
		constructor(options?: TouchOptions);
		close(): void;
		configure(options?: {}): void;
		/** the touch in progress, if any */
		sample(): TouchPoint[];
		target?: any;
		[Symbol.dispose](): void;
	}

	export default Touch;
}

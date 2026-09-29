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

/// <reference path="../config/system.d.ts" />

declare module "embedded:system" {
	/**
	 * Every method is optional: a host provides only those appropriate to it, so test for a method before calling it.
	 */
	interface System {
		restart?(): never;
		shutdown?(): never;
		exit?(reason?: unknown): never;
		/**
		 * @param duration milliseconds
		 */
		sleep?(duration: number): void;
		/**
		 * nRF52 extension. Without a duration, enters System OFF sleep; with a duration, enters System ON sleep
		 * with RAM powered off, waking after `duration` milliseconds. Either way, wake-up reboots.
		 * Returns without sleeping in debug builds.
		 */
		deepSleep?(duration?: number): void;
	}

	const System: Readonly<System>;
	export default System;
}

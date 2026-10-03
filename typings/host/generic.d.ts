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

// a host provider of no particular target: pins by any name, a default bus of each kind in the build, and a network.
// The bus types (I2CBus, ...) are declared by the typing of embedded:provider/builtin
/// <reference path="../embedded_provider/builtin.d.ts" />

declare module "embedded:provider/builtin" {
	import type { PinSpecifier } from "embedded:io/_common";

	interface Device {
		pin: DevicePin;
		network: DeviceNetwork;
	}
	interface DevicePin {
		[name: string]: PinSpecifier;
	}
	interface DeviceAnalog {
		default: AnalogBus;
	}
	interface DeviceI2C {
		default: I2CBus;
	}
	interface DeviceSerial {
		default: SerialBus;
	}
	interface DeviceSPI {
		default: SPIBus;
	}
}

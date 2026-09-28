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

// the bus types (I2CBus, ...) are declared by the typing of embedded:provider/builtin
/// <reference path="../../embedded_provider/builtin.d.ts" />
// the manifest of this target includes the manifests of these configurations
/// <reference path="../../config/digital.d.ts" />
/// <reference path="../../config/i2c.d.ts" />
/// <reference path="../../config/pwm.d.ts" />

declare module "embedded:provider/builtin" {
	import type { PinSpecifier } from "embedded:io/_common";

	interface Device {
		pin: DevicePin;
	}
	interface DevicePin {
		button: PinSpecifier;
		buttonA: PinSpecifier;
		LED: PinSpecifier;
		backlight: PinSpecifier;
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

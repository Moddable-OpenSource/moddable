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

export default function (done) {
	{
		using power = new device.io.SMBus({
			...device.i2c.default,
			hz: 100_000,
			address: 0x6A
		});
		// SY6970: disable the watchdog so it cannot reset the power configuration.
		power.writeUint8(0x07, power.readUint8(0x07) & ~0x30);
		// The display's VBAT supply is wired to BAT, so BATFET must stay on.
		power.writeUint8(0x09, power.readUint8(0x09) & ~0x20);
		// Disable charging for stable USB operation without a battery.
		const value = power.readUint8(0x03);
		power.writeUint8(0x03, value & ~0x10);
	}

	done?.();
}

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

import Expander from "NessoN1Expander";
import SMBus from "embedded:io/smbus";

const VOLTAGE = 0x08;
const CURRENT = 0x0C;
const CHARGER_POWER_CONFIG = 0x01;
const CHARGER_TIMER_CONTROL = 0x05;
const CHARGER_STATUS = 0x08;
const CHARGER_WATCHDOG = 0x60;
const CHARGER_TERMINATION_ENABLE = 1 << 4;
const CHARGER_CONTINUE_AFTER_TERMINATION = 1 << 0;
const CHARGE_DISABLE = 1 << 3;
const HIGH_IMPEDANCE = 1 << 4;
const EXPANDER_INPUT = 0x0F;
const USB_VIN_DETECT = 1 << 5;
const chargeStates = Object.freeze(["not-charging", "precharging", "charging", "complete"]);

export default class Power {
	#gauge;
	#charger;
	#expander;

	constructor() {
		const bus = {...device.i2c.internal, hz: 100_000};
		try {
			this.#gauge = new SMBus({...bus, address: 0x55});
			this.#charger = new SMBus({...bus, address: 0x49, stop: true});
			this.#expander = new Expander(1);
			const charger = this.#charger;
			// Disable the watchdog and stop charging once termination is detected.
			const timer = charger.readUint8(CHARGER_TIMER_CONTROL);
			charger.writeUint8(CHARGER_TIMER_CONTROL,
				(timer & ~(CHARGER_WATCHDOG | CHARGER_CONTINUE_AFTER_TERMINATION)) | CHARGER_TERMINATION_ENABLE);
			charger.writeUint8(CHARGER_POWER_CONFIG, charger.readUint8(CHARGER_POWER_CONFIG) & ~(CHARGE_DISABLE | HIGH_IMPEDANCE));
		}
		catch (error) {
			this.close();
			throw error;
		}
	}

	getBatteryLevel() {
		// Match M5Unified's voltage estimate for Nesso N1. The fuel gauge may
		// retain its default 3000 mAh profile instead of the board's 250 mAh cell.
		const millivolts = this.#gauge.readUint16(VOLTAGE);
		return Math.max(0, Math.min(100, Math.trunc((millivolts - 3300) * 100 / 800)));
	}

	get batteryVoltage() {
		return this.#gauge.readUint16(VOLTAGE) / 1000;
	}

	// Positive current charges the battery; negative current discharges it.
	get batteryCurrent() {
		const value = this.#gauge.readUint16(CURRENT);
		return ((value << 16) >> 16) / 1000;
	}

	getBatteryVoltage() {
		return this.batteryVoltage;
	}

	isVBUSExist() {
		return !!(this.#expander.readUint8(EXPANDER_INPUT) & USB_VIN_DETECT);
	}

	get chargeState() {
		return chargeStates[(this.#charger.readUint8(CHARGER_STATUS) >> 3) & 3];
	}

	get charging() {
		const state = this.chargeState;
		return (state === "precharging") || (state === "charging");
	}

	isCharging() {
		return this.charging;
	}

	close() {
		this.#expander?.close();
		this.#charger?.close();
		this.#gauge?.close();
		this.#expander = this.#charger = this.#gauge = undefined;
	}
}

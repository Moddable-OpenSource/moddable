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

import Timer from "timer";

const Register = Object.freeze({
	POWER_CONFIG: 0x06,
	HOLD_CONFIG: 0x07,
	I2C_CONFIG: 0x09,
	WATCHDOG: 0x0A,
	GPIO_MODE: 0x10,
	GPIO_OUTPUT: 0x11,
	GPIO_INPUT: 0x12,
	GPIO_DRIVE: 0x13,
	GPIO_PULL_0: 0x14,
	GPIO_PULL_1: 0x15,
	GPIO_FUNCTION_0: 0x16,
	GPIO_FUNCTION_1: 0x17,
	IRQ_STATUS_GPIO: 0x40,
	IRQ_STATUS_SYSTEM: 0x41,
	IRQ_STATUS_BUTTON: 0x42,
	IRQ_MASK_GPIO: 0x43,
	IRQ_MASK_SYSTEM: 0x44,
	IRQ_MASK_BUTTON: 0x45,
	BUTTON_STATUS: 0x48,
	BUTTON_CONFIG_1: 0x49,
	BUTTON_CONFIG_2: 0x4A
});

export default class M5PM1 {
	#io;

	constructor(options) {
		const {io: IO, ...dictionary} = options;
		this.#io = new IO({
			...dictionary,
			address: 0x6E,
			hz: 100_000
		});
		this.#io.writeQuick();
		Timer.delay(10);
	}

	close() {
		this.#io?.close();
		this.#io = undefined;
	}

	configure() {
		const io = this.#io;

		// M5PM1 retains some settings while powered by the battery.
		io.writeUint8(Register.I2C_CONFIG, 0); // disable I2C idle sleep
		io.writeUint8(Register.WATCHDOG, 0); // disable watchdog

		// L3B is fed by the 3.3 V LDO. Enable and retain that rail before
		// switching on the LCD/audio load through G2.
		this.#updateBits(Register.POWER_CONFIG, 0b00000100, 0b00000100);
		this.#updateBits(Register.HOLD_CONFIG, 0b00100000, 0b00100000);

		// Begin with all five pins in GPIO mode. G1 becomes the IRQ output below.
		this.#updateBits(Register.GPIO_FUNCTION_0, 0xFF, 0);
		this.#updateBits(Register.GPIO_FUNCTION_1, 0x03, 0);

		// G0: charge status input; G1/G2/G3: outputs; G4: IMU interrupt input.
		this.#updateBits(Register.GPIO_MODE, 0x1F, 0b00001110);
		this.#updateBits(Register.GPIO_DRIVE, 0b00001110, 0); // push-pull outputs
		this.#updateBits(Register.GPIO_PULL_0, 0xFF, 0b00000100); // G1 pull-up
		this.#updateBits(Register.GPIO_PULL_1, 0x03, 0); // G4 has an external pull

		// G1 released high, LCD rail enabled on G2, speaker disabled on G3.
		this.#updateBits(Register.GPIO_OUTPUT, 0b00001110, 0b00000110);
		Timer.delay(100);

		// Route only G4 changes through the M5PM1 IRQ output on G1.
		io.writeUint8(Register.IRQ_STATUS_GPIO, 0);
		io.writeUint8(Register.IRQ_STATUS_SYSTEM, 0);
		io.writeUint8(Register.IRQ_STATUS_BUTTON, 0);
		io.writeUint8(Register.IRQ_MASK_GPIO, 0b00001111);
		io.writeUint8(Register.IRQ_MASK_SYSTEM, 0b00111111);
		io.writeUint8(Register.IRQ_MASK_BUTTON, 0b00000111);
		this.#updateBits(Register.GPIO_FUNCTION_0, 0b00001100, 0b00000100);
	}

	get charging() {
		return !(this.#io.readUint8(Register.GPIO_INPUT) & 0b00000001);
	}
	isCharging() {
		return this.charging;
	}

	get imuInterrupt() {
		return !!(this.#io.readUint8(Register.GPIO_INPUT) & 0b00010000);
	}
	get imuInterruptPending() {
		return !!(this.#io.readUint8(Register.IRQ_STATUS_GPIO) & 0b00010000);
	}
	clearIMUInterrupt() {
		this.#io.writeUint8(Register.IRQ_STATUS_GPIO, 0);
	}

	get buttonPressed() {
		return !!(this.#io.readUint8(Register.BUTTON_STATUS) & 0b00000001);
	}
	getPekState() {
		return this.buttonPressed;
	}
	set singleResetDisabled(disabled) {
		this.#updateBits(Register.BUTTON_CONFIG_1, 0b00000001, disabled ? 0b00000001 : 0);
	}
	set doubleOffDisabled(disabled) {
		this.#updateBits(Register.BUTTON_CONFIG_2, 0b00000001, disabled ? 0b00000001 : 0);
	}

	get l3bEnabled() {
		return !!(this.#io.readUint8(Register.GPIO_OUTPUT) & 0b00000100);
	}
	set l3bEnabled(enabled) {
		this.#setOutput(2, enabled);
	}

	get speakerEnabled() {
		return !!(this.#io.readUint8(Register.GPIO_OUTPUT) & 0b00001000);
	}
	set speakerEnabled(enabled) {
		this.#setOutput(3, enabled);
	}

	#setOutput(pin, enabled) {
		const mask = 1 << pin;
		this.#updateBits(Register.GPIO_OUTPUT, mask, enabled ? mask : 0);
	}

	#updateBits(register, mask, value) {
		const io = this.#io;
		io.writeUint8(register, (io.readUint8(register) & ~mask) | value);
	}
}

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

import config from "mc/config";
import Timer from "timer";

class PowerButton {
	#value = 0;

	read() {
		return this.#value;
	}
	write(value) {
		if (this.#value === value)
			return;
		this.#value = value;
		this.onChanged?.();
	}
}

export default function(done) {
	const pmic = new device.peripheral.Power();
	pmic.configure();
	// M5PM1 retains this setting while powered by the battery.
	pmic.singleResetDisabled = !!config.enablePowerButton;
	pmic.doubleOffDisabled = !!config.enablePowerButton;

	if (config.enablePowerButton) {
		globalThis.power = pmic;
		globalThis.button = {
			power: new PowerButton()
		};
		Timer.repeat(() => {
			globalThis.button.power.write(pmic.getPekState() ? 1 : 0);
		}, 100);
	}
	else
		pmic.close();

	done?.();
}

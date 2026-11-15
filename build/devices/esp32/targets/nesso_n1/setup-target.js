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
import Power from "NessoN1Power";

export default function (done) {
	// Configure USB power and the charger watchdog before enabling Grove.
	const power = new Power();
	power.close();

	const expander = new Expander(1);
	try {
		using grovePower = new expander.Digital({pin: 2, mode: expander.Digital.Output, initial: 1});
	}
	finally {
		// Release the internal bus so applications can use Grove I2C.
		expander.close();
	}

	done?.();
}

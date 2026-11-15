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
import ILI9341 from "ili9341";
import Timer from "timer";

const LCD_RESET = 1 << 1;
const LCD_BACKLIGHT = 1 << 6;
const LCD_OUTPUTS = LCD_RESET | LCD_BACKLIGHT;
export default class extends ILI9341 {
	constructor(options) {
		const expander = new Expander(1);
		try {
			using outputs = new expander.DigitalBank({pins: LCD_OUTPUTS, mode: expander.DigitalBank.Output});
			Timer.delay(10);
			outputs.write(LCD_RESET);
			Timer.delay(120);
			super(options);
			outputs.write(LCD_OUTPUTS);
		}
		finally {
			expander.close();
		}
	}
}

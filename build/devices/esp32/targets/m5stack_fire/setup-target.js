/*
 * Copyright (c) 2020-2026  Moddable Tech, Inc.
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

const G = 9.80665;

export default function (done) {
	if (config.autorotate && globalThis.Application) {
		try {
			const imu = new device.sensor.IMU;
			const threshold = 0.7 * G;
			Timer.repeat(() => {
				if (globalThis.application === undefined)
					return;

				const reading = imu.sample()?.accelerometer;
				if (!reading)
					return;
				if (Math.abs(reading.y) > Math.abs(reading.x)) {
					if (reading.y < -threshold && application.rotation != 90)
						application.rotation = 90;
					else if (reading.y > threshold && application.rotation != 270)
						application.rotation = 270;
				}
				else {
					if (reading.x < -threshold && application.rotation != 180)
						application.rotation = 180;
					else if (reading.x > threshold && application.rotation != 0)
						application.rotation = 0;
				}
			}, config.autorotate);
		}
		catch {
		}
	}

	done?.();
}

/*
 * Copyright (c) 2020-2023  Moddable Tech, Inc.
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

import M5Button from "m5button";
import AudioOut from "pins/audioout";
import Resource from "Resource";
import config from "mc/config";
import Timer from "timer";

export default function (done) {
	global.button = {
		a: new M5Button(39),
		b: new M5Button(38),
		c: new M5Button(37)
	};

	if (config.startupSound) {
	const speaker = new AudioOut({streams: 1});
	speaker.callback = function () {
		this.stop();
		this.close();
		Timer.set(this.done);
	};
		speaker.done = done;
		done = undefined;

		speaker.enqueue(0, AudioOut.Samples, new Resource(config.startupSound));
		speaker.enqueue(0, AudioOut.Callback, 0);
		speaker.start();
	}

	//@@ microphone

	done?.();
}

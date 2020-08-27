/*
 * Copyright (c) 2018-2020  Moddable Tech, Inc.
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

export class Mixer @ "xs_audioout_destructor" {
	constructor(dictionary) @ "xs_audioout";
	close() @ "xs_audioout_close";
	enqueue(stream, kind, buffer, repeat, offset, count) @ "xs_audioout_enqueue";
	mix(samples) @ "xs_audioout_mix";
}
Mixer.Samples = 1;
Mixer.Flush = 2;
Mixer.Callback = 3;
Mixer.Volume = 4;
Mixer.RawSamples = 5;

function build(dictionary) @ "xs_audioout_build";

class AudioOut extends Mixer {
	constructor(dictionary) {
		super(dictionary);
		build.call(this, dictionary);
	}
	start() @ "xs_audioout_start"
	stop() @ "xs_audioout_stop"
	get mix() {}		// unavailable
}

export default AudioOut;

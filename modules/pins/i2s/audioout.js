/*
 * Copyright (c) 2018-2021  Moddable Tech, Inc.
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

export class Mixer extends Native("xs_audioout_destructor") {
	constructor(dictionary) { super(); native("xs_audioout").call(this, dictionary); }
	close() { return native("xs_audioout_close").call(this); }
	enqueue(stream, kind, buffer, repeat, offset, count) { return native("xs_audioout_enqueue").call(this, stream, kind, buffer, repeat, offset, count); }
	mix(samples) { return native("xs_audioout_mix").call(this, samples); }
	length(stream) { return native("xs_audioout_length").call(this, stream); }
	get sampleRate() { return native("xs_audioout_get_sampleRate").call(this); }
	get numChannels() { return native("xs_audioout_get_numChannels").call(this); }
	get bitsPerSample() { return native("xs_audioout_get_bitsPerSample").call(this); }
	get streams() { return native("xs_audioout_get_streams").call(this); }
}
Mixer.Samples = 1;
Mixer.Flush = 2;
Mixer.Callback = 3;
Mixer.Volume = 4;
Mixer.RawSamples = 5;
Mixer.Tone = 6;
Mixer.Silence = 7;

function build(dictionary) { return native("xs_audioout_build").call(this, dictionary); }

class AudioOut extends Mixer {
	constructor(dictionary) {
		super(dictionary);
		build.call(this, dictionary);
	}
	start() { return native("xs_audioout_start").call(this); }
	stop() { return native("xs_audioout_stop").call(this); }
	get mix() {}		// unavailable
}

export default AudioOut;

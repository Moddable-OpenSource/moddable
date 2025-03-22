/*
 * Copyright (c) 2024 Moddable Tech, Inc.
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

class AudioIn @ "xs_audioin_destructor" {
	constructor(dictionary) @ "xs_audioin_constructor";
	close() @ "xs_audioin_close";
	read(samples) @ "xs_audioin_read";
	level(samples) @ "xs_audioin_level";
	start() @ "xs_audioin_start";
	stop() @ "xs_audioin_stop";
	
	get format() @ "xs_audioin_get_format";
	set format(it) @ "xs_audioin_set_format";

	get bitsPerSample() @ "xs_audioin_get_bitsPerSample";
	get channels() @ "xs_audioin_get_channels";
	get sampleRate() @ "xs_audioin_get_sampleRate";
	get audioType() {return "LPCM"}
}

export default AudioIn;

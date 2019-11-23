/*
 * Copyright (c) 2018 Moddable Tech, Inc.
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

class Parser {
	constructor(packet) {
		this.buffer = (packet.byteLength >= 12) ? packet : new ArrayBuffer(12);		// too small to be valid
	}
	get id() @ "xs_dnspacket_get_id"
	get flags() @ "xs_dnspacket_get_flags"
	get questions() @ "xs_dnspacket_get_questions"
	get answers() @ "xs_dnspacket_get_answers"
	get authorities() @ "xs_dnspacket_get_authorities"
	get additionals() @ "xs_dnspacket_get_additionals"
	question(index) @ "xs_dnspacket_question"
	answer(index) @ "xs_dnspacket_answer"
	authority(index) {
		return this.answer(this.answers + index);
	}
	additional(index) {
		return this.answer(this.answers + this.authorities + index);
	}
}
Object.freeze(Parser.prototype);

export default Parser;

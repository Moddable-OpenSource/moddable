/*
* Copyright (c) 2021  Moddable Tech, Inc.
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

export default class extends Native("xs_textdecoder_destructor") {
	constructor(label, options) { super(); native("xs_textdecoder").call(this, label, options); };
	decode(buffer) { return native("xs_textdecoder_decode").call(this, buffer); };
	get encoding() { return native("xs_textdecoder_get_encoding").call(this); };
	get ignoreBOM() { return native("xs_textdecoder_get_ignoreBOM").call(this); };
	get fatal() { return native("xs_textdecoder_get_fatal").call(this); };
}

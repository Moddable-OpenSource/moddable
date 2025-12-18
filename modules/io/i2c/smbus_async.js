/*
 * Copyright (c) 2022-2025  Moddable Tech, Inc.
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

import SMBus from "embedded:implementation/smbussync"

SMBus.Async = class extends Native("_xs_smbusasync_destructor") {
	constructor(options) { super(); native("_xs_smbusasync_constructor").call(this, options); }
	close() { return native("_xs_smbusasync_close").call(this); }
    readUint8(register) { return native("_xs_smbusasync_readUint8").call(this, register); }
    readUint16(register, bigEndian) { return native("_xs_smbusasync_readUint16").call(this, register, bigEndian); }
    readBuffer(register, buffer) { return native("_xs_smbusasync_readBuffer").call(this, register, buffer); } 

    writeUint8(register, byte) { return native("_xs_smbusasync_writeUint8").call(this, register, byte); } 
    writeUint16(register, bigEndian) { return native("_xs_smbusasync_writeUint16").call(this, register, bigEndian); }
    writeBuffer(register, buffer) { return native("_xs_smbusasync_writeBuffer").call(this, register, buffer); } 

    sendByte(command) { return native("_xs_smbusasync_sendByte").call(this, command); }
    receiveByte() { return native("_xs_smbusasync_receiveByte").call(this); }

    readQuick() { return native("_xs_smbusasync_readQuick").call(this); }
    writeQuick() { return native("_xs_smbusasync_writeQuick").call(this); }

	get format() {
		return "buffer";
	}
	set format(value) {
		if ("buffer" !== value)
			throw new Error;
	}
}

export default SMBus;

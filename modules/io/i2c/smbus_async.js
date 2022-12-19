/*
 * Copyright (c) 2022  Moddable Tech, Inc.
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

SMBus.Async = class @ "_xs_smbusasync_destructor" {
	constructor(options) @ "_xs_smbusasync_constructor"
	close() @ "_xs_smbusasync_close"
    readUint8(register) @ "_xs_smbusasync_readUint8"
    readUint16(register, bigEndian) @ "_xs_smbusasync_readUint16"
    readBuffer(register, buffer) @ "_xs_smbusasync_readBuffer" 

    writeUint8(register, byte) @ "_xs_smbusasync_writeUint8" 
    writeUint16(register, bigEndian) @ "_xs_smbusasync_writeUint16"
    writeBuffer(register, buffer) @ "_xs_smbusasync_writeBuffer" 

    sendByte(command) @ "_xs_smbusasync_sendByte"
    receiveByte() @ "_xs_smbusasync_receiveByte"

    readQuick() @ "_xs_smbusasync_readQuick"
    writeQuick() @ "_xs_smbusasync_writeQuick"

	get format() {
		return "buffer";
	}
	set format(value) {
		if ("buffer" !== value)
			throw new Error;
	}
}

export default SMBus;

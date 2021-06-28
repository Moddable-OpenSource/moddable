/*
 * Copyright (c) 2019-2021  Moddable Tech, Inc.
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

import I2C from "embedded:io/i2c"

class SMBus {
	#io;
	#stop;
	#byteBuffer = new Uint8Array(1);
	#wordBuffer = new Uint8Array(2);
	#writeWordBuffer = new Uint8Array(3);

    constructor(options) {
        this.#io = new I2C(options);
		if (options.sendStop)
			this.#stop = true;
    }
    close() {
		if (!this.#io)
			return;

		this.#io.close();
		this.#io = undefined;
    }

    readByte(register) {
		const io = this.#io, buffer = this.#byteBuffer;

        buffer[0] = register;
        io.write(buffer, this.#stop);

        io.read(buffer);
        return buffer[0];
    }

    readWord(register, bigEndian) {
		const io = this.#io, buffer = this.#wordBuffer;

        this.#byteBuffer[0] = register;
        io.write(this.#byteBuffer, this.#stop);

        io.read(buffer);
		return bigEndian ? ((buffer[0] << 8) | buffer[1]) : ((buffer[1] << 8) | buffer[0]);
    }

    readBlock(register, buffer) {
		const io = this.#io;

        this.#byteBuffer[0] = register;
        io.write(this.#byteBuffer, this.#stop);

        io.read(buffer);
        return buffer;
    }

    writeByte(register, byte) {
		const io = this.#io, buffer = this.#wordBuffer;

        buffer[0] = register;
        buffer[1] = byte;
        io.write(buffer);
    }

    writeWord(register, word, bigEndian) {
		const io = this.#io, buffer = this.#writeWordBuffer;

        buffer[0] = register;
        if (bigEndian) {
            buffer[1] = word >> 8;
            buffer[2] = word;
        }
        else {
            buffer[1] = word;
            buffer[2] = word >> 8;
        }
        io.write(buffer);
    }

    writeBlock(register, buffer) {
		const io = this.#io;

        this.#byteBuffer[0] = register;
        io.write(this.#byteBuffer, this.#stop);
        io.write(buffer);
    }

	sendByte(command) {
		const io = this.#io, buffer = this.#byteBuffer;

		buffer[0] = command;
		io.write(buffer, this.#stop);
	}

	receiveByte() {
		const io = this.#io, buffer = this.#byteBuffer;

		io.read(buffer, this.#stop);
		return (buffer[0]);
	}

	readQuick() {
		this.#io.read(0, this.#stop);
	}

	writeQuick() {
		this.#io.write(new ArrayBuffer, this.#stop);
	}
}

export default SMBus;

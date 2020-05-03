/*
 * Copyright (c) 2019  Moddable Tech, Inc.
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

import I2C from "builtin/i2c";

class SMBus extends I2C {
    constructor(dictionary){
        super(dictionary);
        this.sendStop = false;
        this.registerBuffer = new Uint8Array(1);
        this.byteBuffer = new Uint8Array(1);
        this.wordBuffer = new Uint8Array(2);
        this.writeWordBuffer = new Uint8Array(3);
        if (dictionary.sendStop !== undefined) this.sendStop = dictionary.sendStop;
    }

    readByte(register) {
        this.registerBuffer[0] = register;
        super.write(this.registerBuffer, this.sendStop);

        super.read(this.byteBuffer);
        return this.byteBuffer[0];
    }

    readWord(register, bigEndian = false) {
        this.registerBuffer[0] = register;
        super.write(this.registerBuffer, this.sendStop);

        super.read(this.wordBuffer);
        if (bigEndian){
            return (this.wordBuffer[0] << 8) | this.wordBuffer[1];
        }else{
            return ((this.wordBuffer[1] << 8) | this.wordBuffer[0]);
        }   
    }

    readBlock(register, buffer){
        this.registerBuffer[0] = register;
        super.write(this.registerBuffer, this.sendStop);

        super.read(buffer);
        return buffer;
    }

    writeByte(register, byte) {
        this.wordBuffer[0] = register;
        this.wordBuffer[1] = byte;
        super.write(this.wordBuffer);
    }

    writeWord(register, word, bigEndian = false){
        this.writeWordBuffer[0] = register;
        if (bigEndian){
            this.writeWordBuffer[1] = (word >> 8) & 0xFF;
            this.writeWordBuffer[2] = word & 0xFF;
        }else{
            this.writeWordBuffer[1] = word & 0xFF;
            this.writeWordBuffer[2] = (word >> 8) & 0xFF;
        }
        super.write(this.writeWordBuffer);
    }

    writeBlock(register, buffer){
        this.registerBuffer[0] = register;
        super.write(this.registerBuffer, false);
        super.write(buffer);
    }
}
Object.freeze(SMBus.prototype);

export default SMBus;

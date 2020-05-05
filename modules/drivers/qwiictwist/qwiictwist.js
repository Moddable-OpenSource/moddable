/*
 * Copyright (c) 2020 Moddable Tech, Inc.
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
/*
	Sparkfun Qwiic Twist Rotary Encoder with Tricolor LED
		https://www.sparkfun.com/products/15083
	Register map: https://cdn.sparkfun.com/assets/learn_tutorials/8/4/6/Qwiic_Twist_Register_Map_-_Fixed_Endianness.pdf
*/

import SMBus from 'pins/smbus';

const REGISTERS = Object.freeze({
    ID: 0x00,
    STATUS: 0x01,
    VERSION: 0x02,
    INTERRUPT_ENABLE: 0x04,
    ENCODER_COUNT: 0x05,
    ENCODER_DIFFERENCE: 0x07,
    TIME_SINCE_MOVEMENT: 0x09,
    TIME_SINCE_CLICK: 0x0B,
    RED: 0x0D,
    GREEN: 0x0E,
    BLUE: 0x0F,
    CONNECT_RED: 0x10,
    CONNECT_GREEN: 0x12,
    CONNECT_BLUE: 0x14,
    TURN_INTERRUPT_TIMEOUT: 0x16,
    I2C_ADDRESS: 0x18
});

const EXPECTED_ID = 0x5C;

export default class Qwiic_Twist extends SMBus {
    #reportTiming = true;

    constructor(dictionary) {
        super({...dictionary, address: 0x3F, timeout: 600 });
        this.buffer = new ArrayBuffer(2);
        this.view = new DataView(this.buffer);

        const id = this.#getID();
        if (id !== EXPECTED_ID) throw new Error(`Part ID Mismatch. Got ${id}. Expected ${EXPECTED_ID}`);
    }

    configure({color, connect, reportTiming} = {}){
        if (color) this.#setRGB(color);
        if (connect) this.#connectRGB(connect);
        if (reportTiming !== undefined) this.#reportTiming = reportTiming;
    }

    sample(){
        let result = {};
        result.status = this.#getStatus();

        if (this.#reportTiming){
            result.msSinceMovement = this.#getTimeSinceMovement();
            result.msSinceClick = this.#getTimeSinceClick();
        }
        
        result.encoderCount = this.#getEncoderCount();
        result.encoderDelta = this.#getAndResetEncoderDelta();
        return result;
    }

    close(){
        super.close();
    }

    #getID(){
        return this.readByte(REGISTERS.ID);
    }

    #getStatus(){
        const status = this.readByte(REGISTERS.STATUS);
        this.writeByte(REGISTERS.STATUS, 0);
        return {
            buttonDown: ((status & 0b0010) !== 0),
            buttonClicked: ((status & 0b0100) !== 0),
            encoderMoved: ((status & 0b0001) !== 0)
        };
    }

    #getEncoderCount(){
        this.readBlock(REGISTERS.ENCODER_COUNT, 2, this.buffer);
        return this.view.getInt16(0, true);
    }

    #getTimeSinceMovement(){
        this.readBlock(REGISTERS.TIME_SINCE_MOVEMENT, 2, this.buffer);
        return this.view.getUint16(0, true);
    }

    #getTimeSinceClick(){
        this.readBlock(REGISTERS.TIME_SINCE_CLICK, 2, this.buffer);
        return this.view.getUint16(0, true);
    }

    #getAndResetEncoderDelta(){
        this.readBlock(REGISTERS.ENCODER_DIFFERENCE, 2, this.buffer);
        this.writeWord(REGISTERS.ENCODER_DIFFERENCE, 0);
        return this.view.getInt16(0, true);
    }
    

    #getVersion() {
        this.readBlock(REGISTERS.VERSION, 2, this.buffer);
        return `${this.view.getUint8(0)}.${this.view.getUint8(1)}`;
    }

    #setRGB({ red = 0, green = 0, blue = 0 }) {
        this.writeByte(REGISTERS.RED, red);
        this.writeByte(REGISTERS.GREEN, green);
        this.writeByte(REGISTERS.BLUE, blue);
    }

    #connectRGB({ red = 0, green = 0, blue = 0 }) {
        this.writeByte(REGISTERS.CONNECT_RED, 0);
        this.writeByte(REGISTERS.CONNECT_GREEN, 0);
        this.writeByte(REGISTERS.CONNECT_BLUE, 0);
        this.writeByte(REGISTERS.CONNECT_RED, red);
        this.writeByte(REGISTERS.CONNECT_GREEN, green);
        this.writeByte(REGISTERS.CONNECT_BLUE, blue);
    }
}

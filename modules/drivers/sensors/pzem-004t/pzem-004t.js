/*
 * Copyright (c) 2023 Moddable Tech, Inc.
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

import {CRC16} from "crc";

const Registers = Object.freeze({
    VOLTAGE:        0x0000,
    CURRENT_LOW:    0x0001,
    CURRENT_HIGH:   0x0002,
    POWER_LOW:      0x0003,
    POWER_HIGH:     0x0004,
    ENERGY_LOW:     0x0005,
    ENERGY_HIGH:    0x0006,
    FREQUENCY:      0x0007,
    POWER_FACTOR:   0x0008,
    ALARM_STATUS:   0x0009
});

const Commands = Object.freeze({
    READ: 0x04,
    RESET_ENERGY: 0x42,
    ERROR: 0x84,
    RESET_ERROR: 0xC2
});

const States = Object.freeze({
    DONE: 0,
    WAITING: -1,
    HAVE_ADDRESS: -2,
    READ: -3,
    ERROR: -4
});

const DEFAULT_BUS_ADDRESS = 0xF8;

export default class PZEM0004T {
    #outBuffer = new ArrayBuffer(8);
    #outView = new DataView(this.#outBuffer);
    #crcView = new DataView(this.#outBuffer, 0, 6);
    #inBuffer = new ArrayBuffer(32);
    #inView = new DataView(this.#inBuffer);
    #writeable = 0;
    #ready = false;
    #io;
    #crc;
    #sampleCallback;
    #state = -1;
    #onInBuffer = 0;

    constructor(options) {
        this.#crc = new CRC16(0x8005, 0xFFFF, true, true, 0x0000);
        
        this.#io =  new options.sensor.io({
            baud: 9600,
            format: "buffer",
            ...options.sensor,
            onWritable: bytes => {
                this.#writeable = bytes;
                if (!this.#ready && bytes >= 8) {
                    this.#ready = true;
                    if (this.#sampleCallback !== undefined)
                        this.#readRegisters(Registers.VOLTAGE, 10);            
                }
            },
            onReadable: bytes => {
                let result, value;

                this.#io.format = "number";
                try {
                    value = this.#io.read();
                    while ((value !== undefined && result === undefined)){
                        this.#inView.setUint8(this.#onInBuffer++, value);
                        switch (this.#state) {
                            case States.WAITING:
                                this.#state = States.HAVE_ADDRESS;
                                break;
                            case States.HAVE_ADDRESS:
                                if (value === Commands.READ) {
                                    this.#state = States.READ;
                                } else if (value === Commands.ERROR) {
                                    this.#state = States.ERROR;
                                } else if (value === Commands.RESET_ENERGY) {
                                    this.#state = 1;
                                } else { // unknown status code
                                    result = {error: "unknown status code"};
                                }
                                break;
                            case States.READ:
                                this.#state = value + 1;
                                break;
                            case States.ERROR:
                                this.#state = 1;
                                break;
                            case States.DONE:
                                this.#crc.reset();
                                const inCRCView = new DataView(this.#inBuffer, 0, this.#onInBuffer - 2);
                                const calculatedCRC = this.#crc.checksum(inCRCView);
                                const busCRC = this.#inView.getUint16(this.#onInBuffer - 2, true);
                                if (calculatedCRC === busCRC) {
                                    result = this.#handleData();
                                } else {
                                    result = {error: "bad CRC check"};
                                }
    
                                this.#onInBuffer = 0;
                                this.#state = States.WAITING;
                                break;
                            default:
                                this.#state--;
                                break;
                        }
                        value = this.#io.read();
                    }
                } catch (error) {
                    result = {error: "Serial IO error"};
                }
                
                if (result) {
                    const {error, sample, resetEnergy} = result;
                    if (error) {
                        this.#sampleCallback?.(new Error(error));
                    } else {
                        this.#sampleCallback?.(null, sample);
                    }

                    if (resetEnergy)
                        trace(`Received reset energy ACK\n`);
                    this.#sampleCallback = undefined;
                }                
            }
        });
    }

    sample(callback) {
        if (this.#sampleCallback !== undefined)
            throw new Error("sample pending");
        if (callback === undefined)
            throw new Error("sample callback required");

        this.#sampleCallback = callback;
        if (this.#ready)
            this.#readRegisters(Registers.VOLTAGE, 10);
    }

    // @@ TODO: set alarm
    configure(options = {}) {
        if (true === options.resetEnergy) {
            this.#sendCommand(Commands.RESET_ENERGY);
        }
    }

    close(closeCallback) {
        this.#io?.close();
        this.#io = undefined;
        if (closeCallback)
            Timer.set(() => closeCallback(null));
    }

    get configuration() {
        return { };
    }
    
    get identification() {
        return {
            model: "Peacefair PZEM-004t",
            classification: "Energy"
        }
    }
    
    //@@ Todo: read address & read alarm values
    #handleData() {
        const view = this.#inView;

        const status = view.getUint8(1);
        const length = view.getUint8(2);

        if (status === Commands.READ && length === 20) { // full read value case
            const voltage = (view.getUint16(3, false)) / 10;
            const current = ((view.getUint16(7, false) << 16) | view.getUint16(5, false)) / 1000;
            const power = ((view.getUint16(11, false) << 16) | view.getUint16(9, false)) / 10;
            const energy = ((view.getUint16(15, false) << 16) | view.getUint16(13, false));
            const frequency = view.getUint16(17, false) / 10;
            const powerFactor = view.getUint16(19, false) / 100;
            const alarmStatus = (view.getUint16(21, false) === 0xFFFF);

            return {sample: {voltage, current, power, energy, frequency, powerFactor, alarmStatus}};
        } else if (status === Commands.ERROR) { // error case
            return {error: `error code: ${view.getUint8(3)}`};
        } else if (status === Commands.RESET_ENERGY) {
            return {resetEnergy: true};
        } else { // unknown case
            return {error: "invalid read code"};
        }
    }

    // @@ Currently only used for resetting energy accumulator and could be pre-cooked for that purpose. Leaving to later implement calibration and address set/reset.
    #sendCommand(command, busAddress = DEFAULT_BUS_ADDRESS) {
        const buffer = new ArrayBuffer(4);
        const toCRC = new DataView(buffer, 0, 2);
        const toSend = new DataView(buffer);

        toCRC.setUint8(0, busAddress);
        toCRC.setUint8(1, command);
    
        this.#crc.reset();
        const checksum = this.#crc.checksum(toCRC);
        toSend.setUint16(2, checksum, true);
        
        this.#io.format = "buffer";
        this.#io.write(buffer);
    }

    // @@ If we're only ever going to read all the registers at once with a static busAddress, this buffer could be pre-cooked.
    // Leaving this for now in case we want to break out register subsets for reading or support multiple modbus addresses.
    #readRegisters(registerAddress, count, busAddress = DEFAULT_BUS_ADDRESS) {
        if (this.#writeable < 8)
            throw new Error("TX buffer is full");

        this.#outView.setUint8(0, busAddress);
        this.#outView.setUint8(1, Commands.READ);
        this.#outView.setUint16(2, registerAddress);
        this.#outView.setUint16(4, count);

        this.#crc.reset();
        const checksum = this.#crc.checksum(this.#crcView);
        this.#outView.setUint16(6, checksum, true);  

        this.#io.format = "buffer";
        this.#io.write(this.#outBuffer);
    }
}

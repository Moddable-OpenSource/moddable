/*
 * Copyright (c) 2021 Moddable Tech, Inc.
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
    AudioOut for ES8311 Audio Codec 
    This implementation is based on Espressif example code: https://github.com/espressif/esp-dev-kits/blob/master/esp32-s2-kaluga-1/components/es8311/es8311.c
*/

import {default as AudioOut} from "true/audioout";
import SMBus from "pins/smbus";
import Digital from "pins/digital";
import config from "mc/config";

const Registers = {
    RESET_REG00: 0x00,

    CLK_MANAGER_REG01: 0x01,
    CLK_MANAGER_REG02: 0x02,
    CLK_MANAGER_REG03: 0x03,
    CLK_MANAGER_REG04: 0x04,
    CLK_MANAGER_REG05: 0x05,
    CLK_MANAGER_REG06: 0x06,
    CLK_MANAGER_REG07: 0x07,
    CLK_MANAGER_REG08: 0x08,

    SDPIN_REG09: 0x09,
    SDPOUT_REG0A: 0x0A,

    SYSTEM_REG0B: 0x0B,
    SYSTEM_REG0C: 0x0C,
    SYSTEM_REG0D: 0x0D,
    SYSTEM_REG0E: 0x0E,
    SYSTEM_REG0F: 0x0F,
    SYSTEM_REG10: 0x10,
    SYSTEM_REG11: 0x11,
    SYSTEM_REG12: 0x12,
    SYSTEM_REG13: 0x13,
    SYSTEM_REG14: 0x14,

    ADC_REG15: 0x15,
    ADC_REG16: 0x16,
    ADC_REG17: 0x17,
    ADC_REG18: 0x18,
    ADC_REG19: 0x19,
    ADC_REG1A: 0x1A,
    ADC_REG1B: 0x1B,
    ADC_REG1C: 0x1C,
   
    DAC_REG31: 0x31,
    DAC_REG32: 0x32,
    DAC_REG33: 0x33,
    DAC_REG34: 0x34,
    DAC_REG35: 0x35,
    DAC_REG37: 0x37,

    GPIO_REG44: 0x44,
    GP_REG45: 0x45,
   
    CHD1_REGFD: 0xFD,
    CHD2_REGFE: 0xFE,
    CHVER_REGFF: 0xFF,
    CHD1_REGFD: 0xFD
}
Object.freeze(Registers);

const CONFIGURATION = {
    pre_div: 0x01, 
    pre_multi: 0x08, 
    adc_div: 0x01,
    dac_div: 0x01, 
    fs_mode: 0x00, 
    lrck_h: 0x00,
    lrck_l: 0xff, 
    bclk_div: 0x04,
    adc_osr: 0x10, 
    dac_osr: 0x10
}
Object.freeze(CONFIGURATION);

class ES8311 extends SMBus {
    #powerPin;
    
	constructor(dictionary) {
		super(dictionary.i2c);
        this.#powerPin = new Digital({
            pin: dictionary.power.pin,
            mode: Digital.Output
        });

        const initOutput = dictionary.mode & ES8311.Output;
        const initInput = dictionary.mode & ES8311.Input;
        const isDigitalMic = dictionary.isDigitalMic;

        super.writeByte(Registers.RESET_REG00, 0x00);
        super.writeByte(Registers.GP_REG45, 0x00);
        super.writeByte(Registers.CLK_MANAGER_REG01, 0x30);
        super.writeByte(Registers.CLK_MANAGER_REG02, 0x00);
        
        if (initInput) {
            super.writeByte(Registers.CLK_MANAGER_REG03, 0x10);
            super.writeByte(Registers.ADC_REG16, 0x24);
        }
        
        if (initOutput)
            super.writeByte(Registers.CLK_MANAGER_REG04, 0x10);
        
        super.writeByte(Registers.CLK_MANAGER_REG05, 0x00);
        super.writeByte(Registers.SYSTEM_REG0B, 0x00);
        super.writeByte(Registers.SYSTEM_REG0C, 0x00);
        super.writeByte(Registers.SYSTEM_REG10, 0x1F);
        super.writeByte(Registers.SYSTEM_REG11, 0x7F);
        super.writeByte(Registers.RESET_REG00, 0x80);

        let value = super.readByte(Registers.RESET_REG00);
        value &= 0xBF;
        super.writeByte(Registers.RESET_REG00, value);
        super.writeByte(Registers.SYSTEM_REG0D, 0x01);
        super.writeByte(Registers.CLK_MANAGER_REG01, 0x3F);

        value = super.readByte(Registers.CLK_MANAGER_REG01);
        value |= 0x80;
        super.writeByte(Registers.CLK_MANAGER_REG01, value);

        value = super.readByte(Registers.CLK_MANAGER_REG02) & 0x07;
        value |= (CONFIGURATION.pre_div - 1) << 5;
        const datmp = 3;

        value |= (datmp) << 3;
        super.writeByte(Registers.CLK_MANAGER_REG02, value);

        value = super.readByte(Registers.CLK_MANAGER_REG05) & 0x00;
        value |= (CONFIGURATION.adc_div - 1) << 4;
        value |= (CONFIGURATION.dac_div - 1) << 0;
        super.writeByte(Registers.CLK_MANAGER_REG05, value);

        value = super.readByte(Registers.CLK_MANAGER_REG03) & 0x80;
        value |= CONFIGURATION.fs_mode << 6;
        value |= CONFIGURATION.adc_osr << 0;
        super.writeByte(Registers.CLK_MANAGER_REG03, value);

        value = super.readByte(Registers.CLK_MANAGER_REG04) & 0x80;
        value |= CONFIGURATION.dac_osr << 0;
        super.writeByte(Registers.CLK_MANAGER_REG04, value);

        value = super.readByte(Registers.CLK_MANAGER_REG07) & 0xC0;
        value |= CONFIGURATION.lrck_h << 0;
        super.writeByte(Registers.CLK_MANAGER_REG07, value);

        value = super.readByte(Registers.CLK_MANAGER_REG08) & 0x00;
        value |= CONFIGURATION.lrck_l << 0;
        super.writeByte(Registers.CLK_MANAGER_REG08, value);

        value = super.readByte(Registers.CLK_MANAGER_REG06) & 0xE0;

        if (CONFIGURATION.bclk_div < 19) {
            value |= (CONFIGURATION.bclk_div - 1) << 0;
        } else {
            value |= (CONFIGURATION.bclk_div) << 0;
        }

        super.writeByte(Registers.CLK_MANAGER_REG06, value);

        if (initOutput){
            let dac_iface = super.readByte(Registers.SDPIN_REG09) & 0xC0;
            dac_iface |= 0x0c;
            dac_iface &= 0xFC;
            super.writeByte(Registers.SDPIN_REG09, dac_iface);
        }
        
        if (initInput) {
            let adc_iface = super.readByte(Registers.SDPOUT_REG0A) & 0xC0;
            adc_iface |= 0x0c;
            adc_iface &= 0xFC;
            super.writeByte(Registers.SDPOUT_REG0A, adc_iface);
        }
        
        value = super.readByte(Registers.CLK_MANAGER_REG01);
        value &= ~(0x40);
        super.writeByte(Registers.CLK_MANAGER_REG01, value);
        
        value = super.readByte(Registers.CLK_MANAGER_REG06);
        value &= ~(0x20);
        super.writeByte(Registers.CLK_MANAGER_REG06, value);

        if (initInput) {
            super.writeByte(Registers.SYSTEM_REG14, 0x1A);
            if (isDigitalMic) {
                value = super.readByte(Registers.SYSTEM_REG14);
                value |= 0x40;
                super.writeByte(Registers.SYSTEM_REG14, value);
            } else {
                value = super.readByte(Registers.SYSTEM_REG14);
                value &= ~(0x40);
                super.writeByte(Registers.SYSTEM_REG14, value);
            }
        }

        if (initOutput)
            super.writeByte(Registers.SYSTEM_REG12, 0x00);
        
        super.writeByte(Registers.SYSTEM_REG13, 0x10);
        super.writeByte(Registers.SYSTEM_REG0E, 0x02);

        if (initInput) {
            super.writeByte(Registers.ADC_REG15, 0x40);
            super.writeByte(Registers.ADC_REG1B, 0x0A);
            super.writeByte(Registers.ADC_REG1C, 0x6A);
            super.writeByte(Registers.ADC_REG17, 0xBF);
        }
        
        this.paPower(true);
	}

	configure(dictionary) {
        if (dictionary?.volume !== undefined) {
            let volume = dictionary.volume;
            if (volume < 0)
                volume = 0;
            if (volume > 255)
                volume = 255;
            super.writeByte(Registers.DAC_REG32, volume);
        }
    }

    paPower(state){
        this.#powerPin.write(state ? 1 : 0);
    }

    close(){
        this.paPower(false);
        this.#powerPin.close();
        super.close();
    }
}
ES8311.Output = 0b01;
ES8311.Input =  0b10;

class AudioOutKaluga extends AudioOut {
    static #state = { 
        es8311: undefined,
        count: 0
    }

    constructor(options) {
        const state = AudioOutKaluga.#state;
        
        super(options);

        if (state.es8311 === undefined) {
            state.es8311 = new ES8311({
                i2c: {
                    ...config.es8311.i2c,
                    hz: 100000
                },
                power: {
                    ...config.es8311.power
                },
                mode: ES8311.Output
            });
            state.es8311.configure({volume: config.es8311.volume ?? 128});
        }
        state.count++;
    }

    close() {
        const state = AudioOutKaluga.#state;
        super.close();

        state.count--;
        if (state.count <= 0) {
            state.es8311.close();
            state.es8311 = undefined;
        }
    }
}
export default AudioOutKaluga;
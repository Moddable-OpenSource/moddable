/*
 * Copyright (c) 2026  Satoshi Tanaka
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

import AXP192 from "embedded:peripheral/Power/axp192";
import AXP2101 from "embedded:peripheral/Power/axp2101";
import Timer from "timer";

class PowerAXP192 extends AXP192 {
    constructor(options) {
        super(options);
        this.writeByte(0x30, (this.readByte(0x30) & 0x04) | 0x02); //AXP192 30H
        this.writeByte(0x92, this.readByte(0x92) & 0xf8); //AXP192 GPIO1:OD OUTPUT
        this.writeByte(0x93, this.readByte(0x93) & 0xf8); //AXP192 GPIO2:OD OUTPUT
        this.writeByte(0x35, (this.readByte(0x35) & 0x1c) | 0xa3); //AXP192 RTC CHG
    
        // main power line
        this._dcdc1.voltage = 3350;
        this.chargeCurrent = AXP192.CHARGE_CURRENT.Ch_100mA;
    
        // LCD
        this.lcd = this._dcdc3;
        this.lcd.voltage = 2800;
    
        // internal LCD logic
        this._ldo2.voltage = 3300;
        this._ldo2.enable = true;
    
        // Vibration
        this.vibration = this._ldo3;
        this.vibration.voltage = 2000;
    
        // Speaker
        this.speaker = this._gpio2;
    
        // AXP192 GPIO4
        this.writeByte(0x95, (this.readByte(0x95) & 0x72) | 0x84);
        this.writeByte(0x36, 0x4c);
        this.writeByte(0x82, 0xff);
        // reset LCD
        this._gpio4.enable = false
        Timer.delay(20);
        this._gpio4.enable = true
        this.busPowerMode = 0; //  bus power mode_output
        Timer.delay(200);
    }
    set busPowerMode(mode) {
        if (mode === 0) {
            this.writeByte(0x91, (this.readByte(0x91) & 0x0f) | 0xf0);
            this.writeByte(0x90, (this.readByte(0x90) & 0xf8) | 0x02); //set GPIO0 to LDO OUTPUT , pullup N_VBUSEN to disable supply from BUS_5V
            this.writeByte(0x12, this.readByte(0x12) | 0x40); //set EXTEN to enable 5v boost
        } else {
            this.writeByte(0x12, this.readByte(0x12) & 0xbf); //set EXTEN to disable 5v boost
            this.writeByte(0x90, (this.readByte(0x90) & 0xf8) | 0x01); //set GPIO0 to float , using enternal pulldown resistor to enable supply from BUS_5VS
        }
    }
  
    // value 0 - 100 %
    set brightness(value) {
        if (value <= 0)
        value = 2500;
        else if (value >= 100)
        value = 3300;
        else
        value = (value / 100) * 800 + 2500;
        this.lcd.voltage = value;
    }
    
    get brightness() {
        return (this.lcd.voltage - 2500) / 800 * 100;
    }
}

class PowerAXP2101 extends AXP2101 {
    constructor(options) {
        super(options);
        this.writeByte(0x27, 0x00); // PowerKey Hold=1sec / PowerOff=4sec
        this.writeByte(0x10, 0x30); // PMU common config (internal off-discharge enable)
        this.writeByte(0x12, 0x00); // BATFET disable
        this.writeByte(0x68, 0x01); // Battery detection enabled.
        this.writeByte(0x69, 0x13); // CHGLED setting
        this.writeByte(0x99, 0x00); // DLDO1 set 0.5v (vibration motor)
    
        // DCDC1&3  Enable
        this.writeByte(0x80, this.readByte(0x00) | 0x04);
    
        // main power line
        this._dcdc1.voltage = 3350;
    
        // LCD
        this.lcd = this._bldo1;
        this.lcd.voltage = 2800;
        this.lcd.enable = true;
    
        // internal LCD logic
        this._aldo4.voltage = 3300;
        this._aldo4.enable = true;
    
        // Vibration
        this.vibration = this._dldo1;
        this.vibration.voltage = 2000;
    
        // Speaker
        this.speaker = this._aldo3;
        this.speaker.voltage = 3300;
    
        // LCD Reset
        this._aldo2.voltage = 3300;
        this._aldo2.enable = false;
        Timer.delay(20);
        this._aldo2.enable = true;
    
        // bus power mode_output
        this._bldo2.voltage = 3300;
        this._bldo2.enable = true;
        Timer.delay(200);
    }
    // value 0 - 100 %
    set brightness(value) {
        if (value <= 0) value = 2500;
        else if (value >= 100) value = 3300;
        else value = (value / 100) * 800 + 2500;
        this.lcd.voltage = value;
    }

    get brightness() {
        return ((this.lcd.voltage - 2500) / 800) * 100;
    }
}

export default class Core2Power {
    constructor(options) {
        const s = new device.io.SMBus({
            ...device.I2C.internal,
            address: 0x34,
             hz: 400_000
        });
        const powerICID = s.readByte(0x03);
        s.close();
        return powerICID === 0x03 ? new PowerAXP192(options) : new PowerAXP2101(options)
    }
}

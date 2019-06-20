/*
 * Copyright (c) 2019 Shinya Ishikawa
 *
 */
import I2C from 'pins/i2c';

export default class AXP192 extends I2C {
	constructor(it) {
		super(it);
		this.initialize();
	}

	/**
	 * initialize axp192
	 */
	initialize() {
		this.write(0x10, 0xff); // OLED VPP Enable
		this.write(0x28, 0xff); // Enable LDO2&LDO3, LED&TFT 3.3V
		this.write(0x82, 0xff); // Enable all the ADCs
		this.write(0x33, 0xc0); // Enable Charging, 100mA, 4.2V End at 0.9
		this.write(0xB8, 0x80); // Enable Colume Counter
		this.write(0x12, 0x4d); // Enable DC-DC1, OLED VDD, 5B V EXT
		this.write(0x36, 0x5c); // PEK
		this.write(0x90, 0x02); // gpio0
	}

	/**
	 * sets the screen brightness
	 * @param {*} brightness brightness between 7-15
	 */
	setBrightness(brightness) {
		const b = (brightness & 0x0f) << 4;
		this.write(0x28, b);
	}
}

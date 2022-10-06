import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";
import I2C from "embedded:io/i2c";
import SMBus from "embedded:io/smbus";

const device = {
	I2C: {
		default: {
			io: I2C,
			data: 4,
			clock: 5,
			port: 0
		}
	},
	io: { Digital, DigitalBank, I2C, SMBus },
	pin: {
		button: 16,
		buttonA: 16,
		buttonB: 17,
		buttonX: 18,
		buttonY: 19,
		led: 6,
		led_r: 6,
		led_g: 7,
		led_b: 8
	}
};

export default device;


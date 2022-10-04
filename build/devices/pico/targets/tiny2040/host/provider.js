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
		button: 23,
		led: 18,
		led_r: 18,
		led_g: 19,
		led_b: 20
	}
};

export default device;


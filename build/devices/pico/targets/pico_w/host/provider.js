import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";
import I2C from "embedded:io/i2c";
import SMBus from "embedded:io/smbus";

const device = {
	I2C: {
		default: {
			io: I2C,
			data: 2,
			clock: 3,
			port: 1
		}
	},
	io: { Digital, DigitalBank, I2C, SMBus },
	pin: {
		button: 15,
		led: 32
	}
};

export default device;


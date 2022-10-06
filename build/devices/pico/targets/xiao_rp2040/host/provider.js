import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";
import I2C from "embedded:io/i2c";
import SMBus from "embedded:io/smbus";

const device = {
	I2C: {
		default: {
			io: I2C,
			data: 6,
			clock: 7,
			port: 1
		}
	},
	io: { Digital, DigitalBank, I2C, SMBus },
	pin: {
		led: 25
	}
};

export default device;


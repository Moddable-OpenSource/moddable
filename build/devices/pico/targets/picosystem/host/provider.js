import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";

const device = {
	io: { Digital, DigitalBank },
	pin: {
		button: 18,
		buttonA: 18,
		buttonB: 19,
		buttonX: 17,
		buttonY: 16,
		buttonUP: 23,
		buttonDOWN: 20,
		buttonLEFT: 22,
		buttonRIGHT: 21,
		led: 14,
		led_r: 14,
		led_g: 13,
		led_b: 15
	}
};

export default device;


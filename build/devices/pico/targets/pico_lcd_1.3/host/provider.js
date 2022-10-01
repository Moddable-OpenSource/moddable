import Digital from "embedded:io/digital";
import DigitalBank from "embedded:io/digitalbank";

const device = {
	io: {Digital, DigitalBank},
	pin: {
		button: 15,
		buttonA: 15,
		buttonB: 17,
		buttonX: 19,
		buttonY: 21,
		buttonUP: 2,
		buttonDOWN: 18,
		buttonLEFT: 16,
		buttonRIGHT: 20,
		buttonCENTER: 3,
		led: 25
	}
};

export default device;


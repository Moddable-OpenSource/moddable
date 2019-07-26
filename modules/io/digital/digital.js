import DigitalBank from "builtin/digitalbank";

class Digital extends DigitalBank {
	constructor(dictionary) {
		const pins = 1 << dictionary.pin;
		const edge = (undefined === dictionary.edge) ? 0 : dictionary.edge;
		const d = {
			pins,
			mode: dictionary.mode,
			rises: (edge & Digital.Rising) ? pins : 0,
			falls: (edge & Digital.Falling) ? pins : 0,
		};
		if (dictionary.onReadable)
			d.onReadable = dictionary.onReadable;
		super(d);
	}
	read() {
		return super.read() ? 1 : 0;
	}
	write(value) {
		super.write(value ? ~0: 0);
	}
}
Digital.Input = DigitalBank.Input;
Digital.InputPullUp = DigitalBank.InputPullUp;
Digital.InputPullDown = DigitalBank.InputPullDown;
Digital.InputPullUpDown = DigitalBank.InputPullUpDown;

Digital.Output = DigitalBank.Output;
Digital.OutputOpenDrain = DigitalBank.OutputOpenDrain;

Digital.Rising = 1;
Digital.Falling = 2;

export default Digital;

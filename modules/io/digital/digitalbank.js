class DigitalBank @ "xs_digitalbank_destructor" {
	constructor(dictionary) @ "xs_digitalbank_constructor"
	close() @ "xs_digitalbank_close"
	read() @ "xs_digitalbank_read"
	write(value) @ "xs_digitalbank_write"

	get format() {
		return "number";
	}
	set format(value) {
		if ("number" !== value)
			throw new RangeError;
	}
}
DigitalBank.Input = 0;
DigitalBank.InputPullUp = 1;
DigitalBank.InputPullDown = 2;
DigitalBank.InputPullUpDown = 3;

DigitalBank.Output = 8;
DigitalBank.OutputOpenDrain = 9;

export default DigitalBank;

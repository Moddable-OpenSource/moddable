


class Analog @ "xs_analog_destructor" {
	constructor(dictionary) @ "xs_analog_constructor"
	close() @ "xs_analog_close"
	read() @ "xs_analog_read"

	get resolution() {
		return 10;
	}

	get format() {
		return "number";
	}
	set format(value) {
		if ("number" !== value)
			throw new RangeError;
	}
}

export default Analog;

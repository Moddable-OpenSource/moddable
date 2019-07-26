class WakeableDigital @ "xs_wakeabledigital_destructor" {
	constructor(dictionary) @ "xs_wakeabledigital_constructor"
	close() @ "xs_wakeabledigital_close"
	read() @ "xs_wakeabledigital_read"

	get format() {
		return "number";
	}
	set format(value) {
		if ("number" !== value)
			throw new RangeError;
	}
}

export default WakeableDigital;

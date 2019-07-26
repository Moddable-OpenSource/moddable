class I2C @ "_xs_i2c_destructor" {
	constructor(dictionary) @ "_xs_i2c_constructor"
	close() @ "_xs_i2c_close"
	read(count) @ "_xs_i2c_read"
	write(buffer) @ "_xs_i2c_write"

	get format() {
		return "buffer";
	}
	set format(value) {
		if ("buffer" != value)
			throw new RangeError;
	}
}

export default I2C;


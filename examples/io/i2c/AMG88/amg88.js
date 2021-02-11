import SMBus from "embedded:io/smbus";

/*
	register map from SparkFun_GridEYE_Arduino_Library.cpp
*/

const Register = Object.freeze({
	POWER_CONTROL: 0x00,
	RESET: 0x01,
	FRAMERATE: 0x02,
	INT_CONTROL: 0x03,
	STATUS: 0x04,
	STATUS_CLEAR: 0x05,
	AVERAGE: 0x07,
	INT_LEVEL_UPPER: 0x08,
	INT_LEVEL_LOWER: 0x0A,
	INT_LEVEL_HYST: 0x0C,
	THERMISTOR: 0x0E,
	INT_TABLE_INT0: 0x10,
	RESERVED_AVERAGE: 0x1F,
	TEMPERATURE_START: 0x80
});

class AMG88  {
	#io;
	#values = new Uint16Array(16);

	constructor(options) {
		const io = this.#io = new SMBus({
			...options,
			hz: 400_000,	// data sheet says 400 kHz max
			address: 0x69
		});

		io.writeByte(Register.POWER_CONTROL, 0);		// wake
	}
	close() {
		if (!this.#io)
			return;

		this.#io.writeByte(Register.POWER_CONTROL, 0x10);	// sleep

		this.#io?.close();
		this.#io = undefined;
	}
	configure(options) {
	}
	sample() {
		const io = this.#io;
		const result = {};

		result.temperature = convert(io.readWord(Register.THERMISTOR)) * 0.0625;

		const pixels = result.pixels = new Float32Array(64);
		const values = this.#values;
		for (let i = 0; i < 64; i += 16) {
			io.readBlock(Register.TEMPERATURE_START + (i << 1), values.buffer)
			for (let j = 0; j < 16; j++)
				pixels[i + j] = convert(values[j]) * 0.25;
		}

		return result;
	}
}

function convert(value) {
	if (value & (1 << 11))
		return -(value & ~(1 << 11));

	return value;
}

export default AMG88;

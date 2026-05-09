import Timer from "timer";

const SENSITIVITY_REGISTERS = [0x02, 0x03, 0x04, 0x05, 0x06];
const REG_CONFIG = 0x08;
const REG_CONTROL = 0x09;
const REG_REF_RESET1 = 0x0a;
const REG_REF_RESET2 = 0x0b;
const REG_CHANNEL_HOLD1 = 0x0c;
const REG_CHANNEL_HOLD2 = 0x0d;
const REG_CAL_HOLD1 = 0x0e;
const REG_CAL_HOLD2 = 0x0f;
const REG_OUTPUT1 = 0x10;

const OUTPUT_NONE = 0;

const TYPE_LOW = 0;
const TYPE_HIGH = 1;

const LEVEL_0 = 0;
const LEVEL_7 = 7;
const MIN_CHANNELS = 1;
const MAX_CHANNELS = 12;

const LOW_SENSITIVITY_VALUES = [0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77];
const HIGH_SENSITIVITY_VALUES = [
	0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff,
];

class Si12T {
	#io;
	#active = true;
	#sensitivityType = TYPE_LOW;
	#sensitivityLevel = LEVEL_0;
	#channels = 3;
	#points;

	constructor(options) {
		const {
			sensor,
			reset,
			interrupt,
			onSample,
			sensitivityType = TYPE_LOW,
			sensitivityLevel = LEVEL_0,
			channels = 3,
		} = options;
		const io = new sensor.io({
			hz: 100_000,
			address: 0x68,
			...sensor,
		});
		this.#io = io;

		try {
			if (reset) {
				io.reset = new reset.io({
					...reset,
				});

				io.reset.write(1);
				Timer.delay(5);
				io.reset.write(0);
				Timer.delay(150);
			}

			if (interrupt && onSample) {
				io.interrupt = new interrupt.io({
					...interrupt,
					edge: interrupt.io.Falling,
					onReadable: onSample.bind(this),
				});
			}

			this.#enableChannel();
			this.#setControl2();
			this.#setControl1();
			this.configure({
				active: true,
				sensitivityType,
				sensitivityLevel,
				channels,
			});
		} catch (e) {
			this.close();
			throw e;
		}
	}

	close() {
		this.#io?.reset?.close();
		this.#io?.interrupt?.close();
		this.#io?.close();
		this.#io = undefined;
	}

	/**
	 * Configure the Si12T.
	 *
	 * Options:
	 * - active: true for normal sensing mode, false for sleep mode.
	 * - sensitivityType: 0 for low sensitivity range, 1 for high sensitivity range.
	 * - sensitivityLevel: 0 through 7. Lower values detect smaller capacitance changes.
	 * - channels: number of channels returned by sample(), 1 through 12. Defaults to 3.
	 */
	configure(options) {
		if ("active" in options) {
			const active = options.active;
			this.#active = active;
			if (active) this.#sleepDisable();
			else this.#sleepEnable();
		}

		if ("sensitivityType" in options || "sensitivityLevel" in options) {
			this.#setSensitivity(
				options.sensitivityType ?? this.#sensitivityType,
				options.sensitivityLevel ?? this.#sensitivityLevel,
			);
		}

		if ("channels" in options) {
			this.#channels = this.#validateChannels(options.channels);
			this.#points = new Array(this.#channels).fill(OUTPUT_NONE);
		}
	}

	get configuration() {
		return {
			active: this.#active,
			sensitivityType: this.#sensitivityType,
			sensitivityLevel: this.#sensitivityLevel,
			channels: this.#channels,
		};
	}

	sample() {
		const count = Math.ceil(this.#channels / 4);

		for (let i = 0; i < count; i++)
			this.#parseTouchByte(this.#readRegister(REG_OUTPUT1 + i), i * 4);

		return this.#points;
	}

	#readRegister(register) {
		this.#assertOpen();
		return this.#io.readUint8(register);
	}

	#writeRegister(register, value) {
		this.#assertOpen();
		this.#io.writeUint8(register, value & 0xff);
	}

	#setSensitivityValue(value) {
		for (const register of SENSITIVITY_REGISTERS)
			this.#writeRegister(register, value);
	}

	#setSensitivity(type, level) {
		if (type < TYPE_LOW || type > TYPE_HIGH)
			throw new RangeError("invalid sensitivity type");
		if (level < LEVEL_0 || level > LEVEL_7)
			throw new RangeError("invalid sensitivity level");

		const values =
			type === TYPE_HIGH ? HIGH_SENSITIVITY_VALUES : LOW_SENSITIVITY_VALUES;

		this.#sensitivityType = type;
		this.#sensitivityLevel = level;
		this.#setSensitivityValue(values[level]);
	}

	#setControl1() {
		this.#writeRegister(REG_CONFIG, 0x22);
	}

	#setControl2() {
		this.#writeRegister(REG_CONTROL, 0x0f);
		this.#writeRegister(REG_CONTROL, 0x07);
	}

	#sleepEnable() {
		this.#writeRegister(REG_CONTROL, 0x07);
	}

	#sleepDisable() {
		this.#writeRegister(REG_CONTROL, 0x03);
	}

	#enableChannel() {
		this.#writeRegister(REG_REF_RESET1, 0x00);
		this.#writeRegister(REG_REF_RESET2, 0x00);
		this.#writeRegister(REG_CHANNEL_HOLD1, 0x00);
		this.#writeRegister(REG_CHANNEL_HOLD2, 0x00);
		this.#writeRegister(REG_CAL_HOLD1, 0x00);
		this.#writeRegister(REG_CAL_HOLD2, 0x00);
	}

	#parseTouchByte(value, offset) {
		const limit = Math.min(4, this.#channels - offset);

		for (let i = 0; i < limit; i++) {
			this.#points[offset + i] = (value >> (i * 2)) & 0x03;
		}
	}

	#validateChannels(value) {
		if (value < MIN_CHANNELS || value > MAX_CHANNELS)
			throw new RangeError("invalid channel count");
		return value;
	}

	#assertOpen() {
		if (!this.#io) throw new Error("Si12T is closed");
	}
}

export default Si12T;

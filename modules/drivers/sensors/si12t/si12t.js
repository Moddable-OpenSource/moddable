import Timer from "timer";

const SENSITIVITY_REGISTERS = Uint8Array.of(0x02, 0x03, 0x04, 0x05, 0x06);
const REG_CONFIG = 0x08;
const REG_CONTROL = 0x09;
const REG_REF_RESET1 = 0x0a;
const REG_REF_RESET2 = 0x0b;
const REG_CHANNEL_HOLD1 = 0x0c;
const REG_CHANNEL_HOLD2 = 0x0d;
const REG_CAL_HOLD1 = 0x0e;
const REG_CAL_HOLD2 = 0x0f;
const REG_OUTPUT1 = 0x10;

const TYPE_LOW = 0;
const TYPE_HIGH = 1;

const LEVEL_0 = 0;
const LEVEL_7 = 7;
const MIN_CHANNELS = 1;
const MAX_CHANNELS = 12;

const LOW_SENSITIVITY_VALUES = Uint8Array.of(0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77);
const HIGH_SENSITIVITY_VALUES = Uint8Array.of(
	0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff
);

class Si12T {
	#io;
	#active = true;
	#sensitivityType = TYPE_LOW;
	#sensitivityLevel = LEVEL_0;
	#channels = 3;

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

			// enableChannel
			io.writeUint8(REG_REF_RESET1, 0x00);
			io.writeUint8(REG_REF_RESET2, 0x00);
			io.writeUint8(REG_CHANNEL_HOLD1, 0x00);
			io.writeUint8(REG_CHANNEL_HOLD2, 0x00);
			io.writeUint8(REG_CAL_HOLD1, 0x00);
			io.writeUint8(REG_CAL_HOLD2, 0x00);
			// setControl2
			io.writeUint8(REG_CONTROL, 0x0f);
			io.writeUint8(REG_CONTROL, 0x07);

			// setControl1
			io.writeUint8(REG_CONFIG, 0x22);

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
			if (active) this.#io.writeUint8(REG_CONTROL, 0x03);
			else this.#io.writeUint8(REG_CONTROL, 0x07);
		}

		if ("sensitivityType" in options || "sensitivityLevel" in options) {
			const type = options.sensitivityType ?? this.#sensitivityType;
			const level = options.sensitivityLevel ?? this.#sensitivityLevel;
			if (type < TYPE_LOW || type > TYPE_HIGH)
				throw new RangeError("invalid sensitivity type");
			if (level < LEVEL_0 || level > LEVEL_7)
				throw new RangeError("invalid sensitivity level");

			const values =
				type === TYPE_HIGH ? HIGH_SENSITIVITY_VALUES : LOW_SENSITIVITY_VALUES;
			const value = values[level];

			this.#sensitivityType = type;
			this.#sensitivityLevel = level;
			for (const register of SENSITIVITY_REGISTERS)
				this.#io.writeUint8(register, value);
		}

		if ("channels" in options) {
			const channels = options.channels;
			if (channels < MIN_CHANNELS || channels > MAX_CHANNELS)
				throw new RangeError("invalid channel count");
			this.#channels = channels;
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
		const channels = this.#channels;
		const points = new Array(channels);
		const count = Math.ceil(channels / 4);

		for (let i = 0; i < count; i++) {
			const value = this.#io.readUint8(REG_OUTPUT1 + i);
			const offset = i * 4;
			const limit = Math.min(4, channels - offset);

			for (let j = 0; j < limit; j++)
				points[offset + j] = (value >> (j * 2)) & 0x03;
		}

		return points;
	}


}

export default Si12T;

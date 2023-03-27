import Timer from "timer";

const Register = Object.freeze({
	OXYGEN_DATA: 0x03, ///< register for oxygen data
	USER_SET: 0x08, ///< register for users to configure key value manually
	AUTUAL_SET: 0x09, ///< register that automatically configure key value
	GET_KEY: 0x0A ///< register for obtaining key value
});

const I2C_ADDR = 0x73;

class SEN0322
{
	#io;
	#key;
	#onError;

	constructor(options) {
		this.#io = new options.sensor.io({
			address: I2C_ADDR,
			hz: 100_000,
			...options.sensor
		});
		this.#onError = options.onError
		
	}

	configure(options)
	{
		if (undefined !== options.vol)
		{
			const vol = options.vol;
			const mv = options.mv ?? 0;
			if (vol < 0 || vol > 25)
				throw new RangeError("invalid O2 concentration (must be 0-25)");
			else
			{
				const io = this.#io;
				let keyValue = vol * 10;

				if (mv < 0.000001 && mv > (-0.000001))		
					io.write(Uint8Array.of(Register.USER_SET, keyValue));
				else
				{
					keyValue = (vol / mv) * 1000;
					io.write(Uint8Array.of(Register.AUTUAL_SET, keyValue));
				}
			}
		}
	}
	#setKey()
	{
		const io = this.#io;
		io.write(Uint8Array.of(Register.GET_KEY));
		const curKey = (new Uint8Array(io.read(1)))[0];
		if (curKey)
			this.#key = curKey / 1000.0;
		else
			this.#key = 20.9 / 120.0;
	}
	close()
	{
		this.#io.close();
		this.#io = undefined;
	}
	sample()
	{
		const io = this.#io;
		let ret = {};
		this.#setKey();
		io.write(Uint8Array.of(Register.OXYGEN_DATA));
		Timer.delay(100);
		const rxbuf = new Uint8Array(io.read(3));
		const oxygenPercent = ((this.#key) * ((rxbuf[0]) + (rxbuf[1] / 10.0) + (rxbuf[2] / 100.0)));
		ret.O = oxygenPercent * 10000; // O2 concentration in PPM
		return ret;
	}
	
}
export default SEN0322;

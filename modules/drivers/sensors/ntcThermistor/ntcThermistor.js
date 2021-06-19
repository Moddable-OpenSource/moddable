
//import Analog from "embedded:io/analog";

class NTC_THERMISTOR {
	#io;
	#Rseries = 10000;
	#Rnominal = 10000;
	#Beta = 3435;
	#averaging = 1;
	#pullup = false;

	constructor(options) {
		this.#io = new options.sensor.io(options.sensor);
	}
	configure(options) {
		if (undefined !== options.series_resistance)
			this.#Rseries = options.series_resistance;
		if (undefined !== options.thermistor_resistance)
			this.#Rnominal = options.thermistor_resistance;
		if (undefined !== options.beta)
			this.#Beta = options.beta;
		if (undefined !== options.averaging)
			this.#averaging = options.averaging;
		if (undefined !== options.pullup)
			this.#pullup = options.pullup;
	}
	close() {
		this.#io.close;
		this.#io = undefined;
	}
	sample() {
		const io = this.#io;
		const resolution = 2 ** io.resolution;
		let R;
		let adc = 0;
		for (let i=0; i<this.#averaging; i++)
			adc += io.read();
		adc /= this.#averaging;
		if (this.#pullup)
			R = this.#Rseries / (resolution / adc - 1);
		else
			R = this.#Rseries * (resolution / adc - 1);
		const S = (Math.log(R / this.#Rnominal) / this.#Beta) + (1 / 298.15);
		return { temperature: (1/S) - 273.15 };
	}
}

export default NTC_THERMISTOR;


import { ReadableStream } from "streams";

// TBD: interrupt

export function SensorStreamMixin(Base) {
	return class extends ReadableStream {  
		constructor(dictionary) {
	 		super({
				start(controller) {
					const sensor = new Base({
						...dictionary
					});
					System.setInterval(() => {
						const samples = sensor.sample();
						if (samples) {
							if (Array.isArray(samples))
								samples.forEach(sample => controller.enqueue(sample));
							else
								controller.enqueue(samples);
						}
					}, dictionary.interval);
				}
			})
		}
	};
}

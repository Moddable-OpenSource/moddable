import AudioIn from "audioin"
import Timer from "timer"

let input = new AudioIn;
const readingsPerSecond = 8;
const sampleCount = Math.floor(input.sampleRate / readingsPerSecond);
if (16 !== input.bitsPerSample)
	throw new Error("16 bit samples only");

Timer.repeat(() => {
	const samples = new Int16Array(input.read(sampleCount));

	let total = 0;
	for (let i = 0; i < sampleCount; i++) {
		const sample = samples[i];
		if (sample < 0)
			total -= sample;
		else
			total += sample;
	}

	trace(`Average ${(total / sampleCount) | 0}\n`);
}, 1000 / readingsPerSecond);

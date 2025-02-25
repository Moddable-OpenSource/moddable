/*---
description: 
flags: [async, module]
---*/

import AudioOut from "embedded:io/audioout";

let position = 0;
let total = 0;
let out = new AudioOut({
	onWritable(count) {
		if (!count)
			return $DONE("invalid - count of 0")

		try {
			do {
				let buffer = samples.buffer;
				let use = buffer.byteLength - position;
				if (use > count) use = count;
				this.write(new Uint8Array(buffer, position, use));
				position += use;
				if (position === buffer.byteLength)
					position = 0;

				total += use;
				if (total >= (sampleRate * 2 * 2)) {
					this.close();
					$DONE();
					break;
				}

				count -= use;
			} while (count);
		}
		catch (e) {
			$DONE(e);
		}
	}
});

const {sampleRate, channels, bitsPerSample} = out;

assert.sameValue(bitsPerSample, 16, "test assumes 16-bit samples");

const samples = new Int16Array(sampleRate >> 2);
const scaler = (sampleRate / (Math.PI * 2)) * 440;
for (let i = 0, length = samples.length; i < length; i++)
	samples[i] = Math.sin(i * scaler) * 32767;

out.start();

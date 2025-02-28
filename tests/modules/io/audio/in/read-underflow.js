/*---
description: 
flags: [async, module]
---*/

import AudioIn from "embedded:io/audio/in";

let total = 0;
let input = new AudioIn({
	onReadable(bytes) {
		if (bytes <= 0)
			return $DONE(`invalid - bytes of ${bytes}`)

		try {
			do {
				total += this.read(new Uint8Array(8));
				bytes -= 8;

				let samples = this.read();
				assert.sameValue(this.read(), undefined, "nothing to read immedately after full read")
				total += samples.byteLength;
				assert(bytes <= samples.byteLength, "read less than expected");
				if (total >= (sampleRate * 2 * 2)) {
					this.close();
					$DONE();
					break;
				}

				bytes -= samples.byteLength;
			} while (bytes > 0);
		}
		catch (e) {
			$DONE(e);
		}
	}
});

const {sampleRate, channels, bitsPerSample} = input;

assert.sameValue(bitsPerSample, 16, "test assumes 16-bit samples");

input.start();
assert.sameValue(input.read(), undefined, "nothing to read immedately after start")

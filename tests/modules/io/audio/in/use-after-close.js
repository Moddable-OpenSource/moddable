/*---
description: 
flags: [async, module]
---*/

import AudioIn from "embedded:io/audio/in";

let input = new AudioIn({
	onReadable(count) {
		$DONE("never happen");
	}
});
input.close();
assert.throws(SyntaxError, () => input.read(), "read");
assert.throws(SyntaxError, () => input.start(), "start");
assert.throws(SyntaxError, () => input.stop(), "stop");
assert.throws(SyntaxError, () => input.sampleRate, "sampleRate");
assert.throws(SyntaxError, () => input.channels, "channels");
assert.throws(SyntaxError, () => input.bitsPerSample, "bitsPerSample");
assert.throws(SyntaxError, () => input.format, "format");

input = new AudioIn({
	onReadable() {
		$DO(() => {
			this.close();
			assert.throws(SyntaxError, () => this.read(), "read in callback");
		})();
	}
});

input.start();

/*---
description: 
flags: [async, module]
---*/

import AudioOut from "embedded:io/audioout";

let out = new AudioOut({
	onWritable(count) {
		$DONE("never happen");
	}
});
out.close();
assert.throws(SyntaxError, () => out.write(), "write");
assert.throws(SyntaxError, () => out.start(), "start");
assert.throws(SyntaxError, () => out.stop(), "stop");
assert.throws(SyntaxError, () => out.sampleRate, "sampleRate");
assert.throws(SyntaxError, () => out.channels, "channels");
assert.throws(SyntaxError, () => out.bitsPerSample, "bitsPerSample");
assert.throws(SyntaxError, () => out.format, "format");

out = new AudioOut({
	onWritable(count) {
		$DO(() => {
			this.close();
			assert.throws(SyntaxError, () => this.write(), "write in callback");
		})();
	}
});

out.start();

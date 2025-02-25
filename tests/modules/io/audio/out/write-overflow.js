/*---
description: 
flags: [async, module]
---*/

import AudioOut from "embedded:io/audioout";

let out = new AudioOut({
	onWritable(bytes) {
		$DO(() => {
			bytes += (bytes >> 2) & ~3;
			assert.throws(Error, () => this.write(new ArrayBuffer(bytes)));
			this.close();
		})();
	}
});

out.start();

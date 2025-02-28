/*---
description: 
flags: [async, module]
---*/

import AudioOut from "embedded:io/audioout";

function callWithInvalidReceivers(obj, functionName, ...args)
{
	assert.throws(SyntaxError, () => obj[functionName].apply(new $TESTMC.HostObjectChunk, args), `${functionName} with HostObjectChunk`);
	assert.throws(SyntaxError, () => obj[functionName].apply(new $TESTMC.HostObject, args), `${functionName} with HostObject`);
	assert.throws(SyntaxError, () => obj[functionName].apply("a string", args), `${functionName} with string`);
	assert.throws(SyntaxError, () => obj[functionName].apply([], args), `${functionName} with array`);
}

let out = new AudioOut({
	onWritable(count) {
		$DO(() => {
			callWithInvalidReceivers(this, "write", new ArrayBuffer(16));
			callWithInvalidReceivers(this, "close");
			this.close();
		})();
	}
});

callWithInvalidReceivers(out, "start");
out.start();
callWithInvalidReceivers(out, "stop");

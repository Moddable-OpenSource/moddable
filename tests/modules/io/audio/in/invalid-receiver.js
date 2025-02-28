/*---
description: 
flags: [async, module]
---*/

import AudioIn from "embedded:io/audioin";

function callWithInvalidReceivers(obj, functionName, ...args)
{
	assert.throws(SyntaxError, () => obj[functionName].apply(new $TESTMC.HostObjectChunk, args), `${functionName} with HostObjectChunk`);
	assert.throws(SyntaxError, () => obj[functionName].apply(new $TESTMC.HostObject, args), `${functionName} with HostObject`);
	assert.throws(SyntaxError, () => obj[functionName].apply("a string", args), `${functionName} with string`);
	assert.throws(SyntaxError, () => obj[functionName].apply([], args), `${functionName} with array`);
}

let input = new AudioIn({
	onReadable(count) {
		$DO(() => {
			callWithInvalidReceivers(this, "read", new ArrayBuffer(16));
			callWithInvalidReceivers(this, "close");
			this.close();
		})();
	}
});

callWithInvalidReceivers(input, "start");
input.start();
callWithInvalidReceivers(input, "stop");

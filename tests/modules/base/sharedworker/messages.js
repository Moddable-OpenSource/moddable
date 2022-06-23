/*---
description: 
flags: [module,async]
---*/

import {SharedWorker} from "worker";

const minimumOptions = {
	allocation: 8192,
	stackCount: 64,
	slotCount: 64,
	keyCount: 7
};

const s = [], count = 4;
let connected = 0; 
let messageCount = 0;
for (let i = 0; i < count; i++) {
	s[i] = new SharedWorker("testsharedworker", minimumOptions);
	s[i].port.onmessage = function (message) {
		if (undefined === message.connectedCount)
			return $DONE("unexpected connectedCount");
		connected++;
		if (connected !== message.connectedCount) 
			return $DONE("out of order connection");
		if (connected === count)
			stepTwo();
	}
}

assert.throws(SyntaxError, () => s[0].port.postMessage(), "postMessage requires 1 argument");
assert.throws(SyntaxError, () => s[0].port.postMessage.call(new $TESTMC.HostObject, 0, 64), "postMessage with non-SharedWorker this");

function stepTwo() {
	for (let i = count - 1; i >= 0; i--) {
		s[i].port.onmessage = function (message) {
			if ((count - i) !== message.messageCount)
				return $DONE("out of order message");
			if (i !== message.value)
				return $DONE("unexpected message value");
			if (0 === message.value) {
				messageCount += count;
				stepThree();
			}
		}
		s[i].port.postMessage(i);
	}
}

function stepThree() {
	for (let i = 0; i < count; i++) {
		s[i].port.postMessage(String(i));
		if (i === (count - 1)) {
			s[i].port.onmessage = function (message) {
				if ((messageCount + count) !== message.messageCount)
					return $DONE("out of order message");
				if (String(i) !== message.value)
					return $DONE("unexpected message value");

				messageCount += count;
				stepFour();
			}
		}
		else
			delete s[i].port.onmessage;
	}
}


function stepFour() {
	$DONE();
} 

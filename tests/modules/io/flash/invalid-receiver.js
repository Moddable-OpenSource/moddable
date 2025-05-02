/*---
description: 
flags: [module]
---*/

import flash from "./flash-FIXTURE.js";

function callWithInvalidReceivers(obj, functionName, ...args)
{
	assert.throws(SyntaxError, () => obj[functionName].apply(new $TESTMC.HostObjectChunk, args), `${functionName.toString()} with HostObjectChunk`);
	assert.throws(SyntaxError, () => obj[functionName].apply(new $TESTMC.HostObject, args), `${functionName.toString()} with HostObject`);
	assert.throws(SyntaxError, () => obj[functionName].apply("a string", args), `${functionName.toString()} with string`);
	assert.throws(SyntaxError, () => obj[functionName].apply([], args), `${functionName.toString()} with array`);
}

import {path} from "./flash-fixture.js";

let f = flash.open({path}); 

callWithInvalidReceivers(f, "write", Uint8Array.of(1), 0);
callWithInvalidReceivers(f, "read", 5, 0);
callWithInvalidReceivers(f, "eraseBlock", 0);
callWithInvalidReceivers(f, "status");
callWithInvalidReceivers(f, "close");

f.close();

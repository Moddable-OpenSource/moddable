/*---
description: 
flags: [module]
---*/

const names = [ "resolveHook", "loadHook", "loadNowHook" ];

function test(name, hook) {
	return new Compartment({ [name]: hook });
}
 
assert.sameValue((new Compartment()).toString(), "[object Compartment]", "no options");
assert.sameValue((new Compartment({})).toString(), "[object Compartment]", "no hooks");
for (let name of names) {
	assert.sameValue(test(name, function(){}).toString(), "[object Compartment]", name + ": function");
	assert.sameValue(test(name, new Proxy(function(){}, {})).toString(), "[object Compartment]", name + ": function proxy");

	assert.throws(TypeError, () => test(name, undefined), name + ": undefined");
	assert.throws(TypeError, () => test(name, null), name + ": null");
	assert.throws(TypeError, () => test(name, false), name + ": boolean");
	assert.throws(TypeError, () => test(name, 0), name + ": number");
	assert.throws(TypeError, () => test(name, ""), name + ": string");
	assert.throws(TypeError, () => test(name, Symbol()), name + ": symbol");
	assert.throws(TypeError, () => test(name, {}), name + ": object");
	assert.throws(TypeError, () => test(name, []), name + ": array");
	assert.throws(TypeError, () => test(name, new Proxy({}, {})), name + ": proxy");
}
/*---
description: 
flags: [module]
---*/

import storage from "./storage_FIXTURE.js";
import {emptyDomain, keys} from "./storage_FIXTURE.js";

function supportsFormat(store, format) {
	try {
		store.format = format;
	}
	catch {
	}
	return store.format === format;
}

const path = "test";
let store = storage.open({path}); 

if (supportsFormat(store, "buffer")) {
	const verify = function() {
		let result = store.read("buffer");
		assert(result instanceof ArrayBuffer, "expect ArrayBuffer");
		assert.sameValue(result.byteLength, 2, "expect length 2");
		result = new Uint8Array(result);
		assert(!result[0] && !result[1], "expect all zeros");
	}
	store.write("buffer", Uint8Array.of(0, 0)); verify();
	store.write("buffer", new ArrayBuffer(2)); verify();
	store.write("buffer", new DataView(new ArrayBuffer(2))); verify();
	store.write("buffer", Uint8Array.of(1, 0, 0).subarray(1)); verify();

	assert.sameValue(store.read("buffer-x"), undefined);
}

if (supportsFormat(store, "string")) {
	const test = function(value) {
		store.write("string", value);
		assert.sameValue(store.read("string"), value);
	}
	test("123");
	test("");
	test("with a space");
	test("Môddable");
	test("xy\nzzy");

	assert.sameValue(store.read("string-x"), undefined);
}

if (supportsFormat(store, "uint8")) {
	const test = function(value) {
		store.write("uint8", value);
		assert.sameValue(store.read("uint8"), Uint8Array.of(value)[0]);
	}
	test(0);
	test(255);
	test(-1);
	test(256);
	test(257);

	assert.sameValue(store.read("uint8-x"), undefined);
}

if (supportsFormat(store, "int8")) {
	const test = function(value) {
		store.write("int8", value);
		assert.sameValue(store.read("int8"), Int8Array.of(value)[0]);
	}
	test(0);
	test(-128);
	test(-129);
	test(127);
	test(128);
	test(129);

	assert.sameValue(store.read("int8-x"), undefined);
}

if (supportsFormat(store, "uint16")) {
	const test = function(value) {
		store.write("uint16", value);
		assert.sameValue(store.read("uint16"), Uint16Array.of(value)[0]);
	}
	test(0);
	test(65535);
	test(65536);
	test(-1);

	assert.sameValue(store.read("uint16-x"), undefined);
}

if (supportsFormat(store, "int16")) {
	const test = function(value) {
		store.write("int16", value);
		assert.sameValue(store.read("int16"), Int16Array.of(value)[0]);
	}
	test(-32768);
	test(32767);
	test(32769);
	test(-32769);

	assert.sameValue(store.read("int16-x"), undefined);
}

if (supportsFormat(store, "uint32")) {
	const test = function(value) {
		store.write("uint32", value);
		assert.sameValue(store.read("uint32"), Uint32Array.of(value)[0]);
	}
	test(0);
	test(0xffffffff);
	test(0x100000000);
	test(-1);

	assert.sameValue(store.read("uint32-x"), undefined);
}

if (supportsFormat(store, "int64")) {
	const test = function(value) {
		store.write("int64", value);
		assert.sameValue(store.read("int64"), BigInt64Array.of(value)[0]);
	}
	test(0n);
	test(2147483648000n);
	test(-2147483648000n);

	assert.sameValue(store.read("int64-x"), undefined);
}

if (supportsFormat(store, "uint64")) {
	const test = function(value) {
		store.write("uint64", value);
		assert.sameValue(store.read("uint64"), BigUint64Array.of(value)[0]);
	}
	test(0n);
	test(2147483648000n);
	test(-1n);

	assert.sameValue(store.read("uint64-x"), undefined);
}

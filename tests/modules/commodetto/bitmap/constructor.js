/*---
description: 
flags: [module]
---*/

import Bitmap from "commodetto/Bitmap";

assert.throws(SyntaxError, () => new Bitmap(), "Bitmap constructor requires 5 arguments");
assert.throws(SyntaxError, () => new Bitmap(32), "Bitmap constructor requires 5 arguments");
assert.throws(SyntaxError, () => new Bitmap(32, 32), "Bitmap constructor requires 5 arguments");
assert.throws(SyntaxError, () => new Bitmap(32, 32, Bitmap.Default), "Bitmap constructor requires 5 arguments");
assert.throws(SyntaxError, () => new Bitmap(32, 32, Bitmap.Default, new ArrayBuffer(4096)), "Bitmap constructor requires 5 arguments");

assert.throws(TypeError, () => Bitmap(32, 32, Bitmap.Default, new ArrayBuffer(4096), 0), "Bitmap constructor called as function");

assert.throws(TypeError, () => new Bitmap(Symbol(), 32, Bitmap.Default, new ArrayBuffer(4096), 0), "Bitmap constructor rejects synbol");
assert.throws(TypeError, () => new Bitmap(32, Symbol(), Bitmap.Default, new ArrayBuffer(4096), 0), "Bitmap constructor rejects synbol");
assert.throws(TypeError, () => new Bitmap(32, 32, Symbol(), new ArrayBuffer(4096), 0), "Bitmap constructor rejects synbol");
assert.throws(TypeError, () => new Bitmap(32, 32, Bitmap.Default, Symbol(), 0), "Bitmap constructor rejects synbol");
assert.throws(TypeError, () => new Bitmap(32, 32, Bitmap.Default, new ArrayBuffer(4096), Symbol()), "Bitmap constructor rejects synbol");

assert.throws(Error, () => new Bitmap(32, 32, 0, new ArrayBuffer(4096), 0), "Bitmap constructor invalid format");	// only 0 is rejected
assert.throws(Error, () => new Bitmap(32, 32, Bitmap.Default, new ArrayBuffer(4096), 4096), "Bitmap constructor invalid offset");
assert.throws(Error, () => new Bitmap(32, 32, Bitmap.Default, new ArrayBuffer(4096), -1), "Bitmap constructor invalid offset");

// accepts string
new Bitmap("32", 32, Bitmap.Default, new ArrayBuffer(4096), 0);
new Bitmap(32, "32", Bitmap.Default, new ArrayBuffer(4096), 0);
new Bitmap(32, 32, Bitmap.Default, new ArrayBuffer(4096), "0");

// invokes toPrimitive
const to32 = {
	[Symbol.toPrimitive]() {
		return 32;
	}
};
const to0 = {
	[Symbol.toPrimitive]() {
		return 0;
	}
};
new Bitmap(to32, 32, Bitmap.Default, new ArrayBuffer(4096), 0);
new Bitmap(32, to32, Bitmap.Default, new ArrayBuffer(4096), 0);
new Bitmap(32, 32, Bitmap.Default, new ArrayBuffer(4096), to0);

// accepts various buffer types
new Bitmap(32, 32, Bitmap.Default, new SharedArrayBuffer(4096), 0);
new Bitmap(32, 32, Bitmap.Default, new $TESTMC.HostBuffer(4096), 0);
new Bitmap(32, 32, Bitmap.Default, new Uint8Array(new ArrayBuffer(4096)), 0);
new Bitmap(32, 32, Bitmap.Default, new Int8Array(new SharedArrayBuffer(4096)), 0);
new Bitmap(32, 32, Bitmap.Default, new DataView(new SharedArrayBuffer(4096)), 0);

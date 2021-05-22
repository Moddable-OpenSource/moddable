/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK.
 * 
 *   This work is licensed under the
 *       Creative Commons Attribution 4.0 International License.
 *   To view a copy of this license, visit
 *       <http://creativecommons.org/licenses/by/4.0>.
 *   or send a letter to Creative Commons, PO Box 1866,
 *   Mountain View, CA 94042, USA.
 *
 */

import Instrumentation from "instrumentation";
import Debug from "debug";

let proxyTarget = {}, proxyObject = {};
let buffer = new ArrayBuffer(4);
let promise_a = new Promise(empty), promise_b = new Promise(empty), promises = [promise_a, promise_b];
function* idMaker() {
    let index = 0;
    while (true)
        yield index++;
}

measure("Boolean", () => true);
measure("Number", () => 1);
measure("String `String in ROM`",  () => "String in ROM");
measure("String `fromCharCode(32)`",  () => String.fromCharCode(32));
measure("String `String in ` + 'RAM'",  () => "String in " + "RAM");
measure("Object {}",  () => {});
measure("Object {x: 1}",  () => {return {x: 1}});
measure("Object {x: 1, y: 2}",  () => {return {x: 1, y: 2}});
measure("Object {x: 1, y: 2, z: 3}",  () => {return {x: 1, y: 2, z: 3}});
measure("Date",  () => new Date);
measure("Function () => {}",  () => {return () => {}});
measure("Function closure (1 variable)",  () => {let a = 1; return function() {return a}});
measure("Function closure (2 variables)",  () => {let a = 1, b = 2; return function() {return a + b;}});
measure("Function closure (3 variables)",  () => {let a = 1, b = 2, c = 3; return function() {return a + b + c;}});
measure("Generator",  () => idMaker());
measure("Error",  () => new Error);
measure("SyntaxError",  () => new SyntaxError);
measure("Promise",  () => new Promise(empty));
measure("Promise.race([a, b])",  () => Promise.race(promises));
measure("Promise.all([a, b])",  () => Promise.all(promises));
measure("Proxy",  () => new Proxy(proxyTarget, proxyObject));
measure("Array []",  () => []);
measure("Array [1]",  () => [1]);
measure("Array [1, 2]",  () => [1, 2]);
measure("Array [1, 2, 3]",  () => [1, 2, 3]);
measure("Array [1, 2, 3, 4, 5, 6, 7, 8, 9]",  () => [1, 2, 3, 4, 5, 6, 7, 8, 9]);
measure("Array.fill length 9",  () => new Array(9).fill(0));
measure("ArrayBuffer - 4 byte buffer",  () => new ArrayBuffer(4));
measure("ArrayBuffer - 32 byte buffer",  () => new ArrayBuffer(32));
measure("Uint8Array (empty)",  () => new Uint8Array);
measure("Uint8Array (buffer)",  () => new Uint8Array(buffer));
measure("Uint8Array [1]",  () => Uint8Array.from([1]));
measure("Uint8Array [1, 2]",  () => Uint8Array.from([1, 2]));
measure("Uint8Array [1, 2, 3, 4, 5, 6, 7, 8, 9]",  () => Uint8Array.from([1, 2, 3, 4, 5, 6, 7, 8, 9]));
measure("Uint16Array (empty)",  () => new Uint16Array);
measure("Uint16Array (buffer)",  () => new Uint16Array(buffer));
measure("Uint16Array [1]",  () => Uint16Array.from([1]));
measure("Uint16Array [1, 2]",  () => Uint16Array.from([1, 2]));
measure("Uint16Array [1, 2, 3, 4, 5, 6, 7, 8, 9]",  () => Uint16Array.from([1, 2, 3, 4, 5, 6, 7, 8, 9]));
measure("Uint32Array (empty)",  () => new Uint32Array);
measure("Uint32Array (buffer)",  () => new Uint32Array(buffer));
measure("Uint32Array [1]",  () => Uint32Array.from([1]));
measure("Uint32Array [1, 2]",  () => Uint32Array.from([1, 2]));
measure("Uint32Array [1, 2, 3, 4, 5, 6, 7, 8, 9]",  () => Uint32Array.from([1, 2, 3, 4, 5, 6, 7, 8, 9]));
measure("DataView (buffer)",  () => new DataView(buffer));
measure("Symbol ('foo')",  () => Symbol("foo"));
measure("Symbol ('foo' + 'bar')",  () => Symbol("foo" + "bar"));
measure("Map (empty)",  () => new Map);
measure("Map 1 entry",  () => {let m = new Map; m.set(1, 1); return m;});
measure("Map 2 entries",  () => {let m = new Map; m.set(1, 1); m.set(2, 2); return m;});
measure("WeakMap (empty)",  () => new WeakMap);
measure("Set (empty)",  () => new Set);
measure("Set 1 entry",  () => {let s = new Set; s.add(1); return s;});
measure("Set 2 entry",  () => {let s = new Set; s.add(1); s.add(2); return s;});
measure("WeakSet (empty)",  () => new WeakSet);

function measure(name, what)
{
	let slots, chunks, result = undefined /* {} */;

	Debug.gc();

	slots = Instrumentation.get(11);
	chunks = Instrumentation.get(12);

	result = what();

	Debug.gc();

	slots = Instrumentation.get(11) - slots;
	chunks = Instrumentation.get(12) - chunks;

	if (slots && chunks)
		trace(`${name}: ${slots / 16} slots + ${chunks} chunk bytes = ${slots + chunks} bytes\n`)
	else if (slots)
		trace(`${name}: ${slots / 16} slots = ${slots + chunks} bytes\n`)
	else if (chunks)
		trace(`${name}: ${chunks} chunk bytes = ${slots + chunks} bytes\n`)
	else
		trace(`${name}: 0 bytes\n`)
}

function empty() {}

/*---
description: 
flags: [module]
---*/

import Deflate from "deflate";
import Inflate from "inflate";

import Resource from "Resource";

const text = new Resource("image-info.txt");
const circleish = new Resource("circleish-alpha.bm4");
const font = new Resource("OpenSans-Regular-16.bf4");

let def;
try {
	def = new Deflate({});
}
catch {
}
finally {
	assert(def !== undefined, "not enough memory to test Deflate");
	def.close();
	def = undefined;
}

const samples = [text, circleish, font, Uint8Array.of(0, 1, 2), Float32Array.of(0, 0.1, Infinity, -0, Math.PI).buffer];

[8192, 4096, 2048, 1025, 1024, 1023, 513, 511, 510, 256, 128, 64, 32, 1].forEach(chunkSize => {
	testInflate(samples, {chunkSize}, {}, `deflate chunkSize ${chunkSize}`);
});

[8192, 4096, 2048, 1025, 1024, 1023, 513, 511, 510, 256, 128, 64, 32, 1].forEach(chunkSize => {
	testInflate(samples, {}, {chunkSize}, `inflate chunkSize ${chunkSize}`);
});

for (let strategy = 0; strategy <= 4; strategy++)
	testInflate(samples, {strategy}, {}, `strategy ${strategy}`);

for (let level = -1; level <= 10; level++)
	testInflate(samples, {level}, {}, `level ${level}`);

testInflate(samples, {windowBits: 15}, {windowBits: 15}, `windowBits: 15`);
testInflate(samples, {windowBits: -15}, {windowBits: -15}, `windowBits -15`);

function testInflate(samples, deflateOptions, inflateOptions, what) {
	for (let i = 0; i < samples.length; i++) {
		const data = samples[i];

		const deflated = deflate(data, deflateOptions);
		const inflated = inflate(deflated, {...inflateOptions, expected: data.byteLength});
		
		const src = new Uint8Array(ArrayBuffer.isView(data) ? data.buffer : data);
		const dst = new Uint8Array(inflated);

		assert.sameValue(src.byteLength, dst.byteLength, "inflated length should match src length: " + what);

		for (let i = 0, length = src.length; i < length; i++) {
			if (src[i] !== inflated[i])
				assert.sameValue(src[i], inflated[i], "inflated data mismatch: " + what);
		}
	}
}

function deflate(data, options) {
	const deflate = new Deflate(options);
	const chunkSize = options.chunkSize;
	if (undefined === chunkSize)
		deflate.push(data, true);
	else {
		for (let offset = 0; offset < data.byteLength; offset += chunkSize) {
			const use = Math.min(chunkSize, data.byteLength - offset);
			const last = (offset + chunkSize) >= data.byteLength; 
			deflate.push(data.slice(offset, offset + use), last);
		}
	}
	deflate.close();
	return deflate.result;
}

function inflate(data, options) {
	const inflate = new Inflate(options);
	const chunkSize = options.chunkSize;
	if (undefined === chunkSize)
		inflate.push(data, true);
	else if ((data.byteLength / chunkSize) <= 8) {
		for (let offset = 0; offset < data.byteLength; offset += chunkSize) {
			const use = Math.min(chunkSize, data.byteLength - offset);
			const last = (offset + chunkSize) >= data.byteLength; 
			inflate.push(data.slice(offset, offset + use), last);
		}
	}
	else {		// combine partial chunks to minimize heap
		inflate.result = new Uint8Array(options.expected);
		let position = 0;
		inflate.onData = function(chunk) {
			this.result.set(new Uint8Array(chunk), position);
			position += chunk.byteLength;
		}
		inflate.onEnd = function() {
		}

		for (let offset = 0; offset < data.byteLength; offset += chunkSize) {
			const use = Math.min(chunkSize, data.byteLength - offset);
			const last = (offset + chunkSize) >= data.byteLength; 
			inflate.push(data.slice(offset, offset + use), last);
		}
	}
	inflate.close();
	return inflate.result;
}

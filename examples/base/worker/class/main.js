/*
 * Copyright (c) 2016-2025  Moddable Tech, Inc.
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

import { SimpleWorker } from "simpleworker";

trace("main.js: hello\n");

let index = 0

function start() {
	// note: chunk, heap (slots), and stack are very small - most real-world workers will require different settings
	trace("main.js: Starting, creating worker\n");
	const aWorker = new SimpleWorker("coreworker", {
		static: 8 * 1024,		// total bytes for slots and chunks, in bytes
		chunk: {				// size in bytes, for strings, array buffers
			initial: 1536,
			incremental: 256
		},
		heap: {					// in 16 byte slots (32-bit MCU(
			initial: 128,		// 2K bytes (32-bit MCU(
			incremental: 32
		},
		stack: 128				// number of 16 byte slots (32-bit MCU(
	});
	trace("main.js: Worker created\n");
	aWorker.postMessage({hello: "world", index: ++index});
	aWorker.postMessage("hello, again");
	aWorker.postMessage([1, 2, 3]);
	trace("main.js: Messages sent\n");
	aWorker.onmessage = function(message) {
		trace(`main.js: worker -> ${JSON.stringify(message)}\n`);
		if (3 === message.counter) {
			trace(`main.js: restarting worker ${index}\n`);
			aWorker.terminate();
			start();
		}
	}
}

start();

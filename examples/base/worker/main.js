/*
 * Copyright (c) 2016-2023  Moddable Tech, Inc.
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

import Worker from "worker";

trace("hello\n");

let index = 0

function start() {
	// note: chunk, heap (slots), and stack are very small - most real-world workers will require different settings
	const aWorker = new Worker("simpleworker", {
		static: 6 * 1024,
		chunk: {
			initial: 1536,
			incremental: 256
		},
		heap: {
			initial: 64,
			incremental: 32
		},
		stack: 64
	});

	aWorker.postMessage({hello: "world", index: ++index});
	aWorker.postMessage("hello, again");
	aWorker.postMessage([1, 2, 3]);

	aWorker.onmessage = function(message) {
		if (3 === message.counter) {
			trace(`start worker ${index}\n`);
			aWorker.terminate();
			start();
		}
	}
}

start();

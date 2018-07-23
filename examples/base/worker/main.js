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

import Worker from "worker";

let index = 0

function start() {
	let aWorker = new Worker("simpleworker");

	aWorker.postMessage({hello: "world", index: ++index});
	aWorker.postMessage("hello, again");
	aWorker.postMessage([1, 2, 3]);

	aWorker.onmessage = function(message) {
//		trace("main receives message\n");
//		trace(JSON.stringify(message));
//		trace("\n");

		if (3 == message.counter) {
			trace(`start worker ${index}\n`);
			aWorker.terminate();
			start();
		}
	}
}

start();

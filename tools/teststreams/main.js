/*
 * Copyright (c) 2018-2025  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Tools.
 * 
 *   The Moddable SDK Tools is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 * 
 *   The Moddable SDK Tools is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 * 
 *   You should have received a copy of the GNU General Public License
 *   along with the Moddable SDK Tools.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

import {} from "_262";
import structuredClone from "structuredClone";

function flushPromises() { return native("flushPromises").call(this); }
const formerSetTimeout = setTimeout;
globalThis.setTimeout = function(f, ms) {
	return formerSetTimeout(() => {
		flushPromises();
		f();
	}, ms);
}

globalThis.$DO = function(f) {
	return function(...args) {
		try {
			f(...args);
			$DONE();
		}
		catch(e) {
			$DONE(e);
		}
	}
}

import * as domexception from "web/domexception";
for (let key in domexception)
	globalThis[key] = domexception[key];
import * as abortsignal from "web/abortsignal";
for (let key in abortsignal)
	globalThis[key] = abortsignal[key];
import * as streams from "web/streams";
for (let key in streams)
	globalThis[key] = streams[key];
import TextDecoder from "text/decoder";
globalThis.TextDecoder = TextDecoder;
import TextDecoderStream from "web/textdecoderstream";
globalThis.TextDecoderStream = TextDecoderStream;
import TextEncoder from "text/encoder";
globalThis.TextEncoder = TextEncoder;
import TextEncoderStream from "web/textencoderstream";
globalThis.TextEncoderStream = TextEncoderStream;
import DecompressionStream from "web/decompressionstream";
globalThis.DecompressionStream = DecompressionStream;
import Crypto, {SubtleCrypto, CryptoKey} from "webcrypto";
globalThis.crypto = new Crypto;
globalThis.Crypto = Crypto;
globalThis.SubtleCrypto = SubtleCrypto;
globalThis.CryptoKey = CryptoKey;
globalThis.btoa = s => Uint8Array.from(String(s), c => c.charCodeAt(0)).toBase64();
globalThis.atob = s => String.fromCharCode(...Uint8Array.fromBase64(String(s)));
// newer WPT testharness.js; the harness copy here predates QuotaExceededError becoming its own interface
globalThis.assert_throws_quotaexceedederror = (func, requested, quota, description) => assert_throws_dom("QuotaExceededError", func, description);
import config from "mc/config";
globalThis.slowTests = config.slowTests ?? false;		// WPT PBKDF2 1000/100000-iteration cases; enable with mcconfig ... slowTests=true

globalThis.readableStreamFromArray = function(array) {
  return new ReadableStream({
    start(controller) {
      for (let entry of array) {
        controller.enqueue(entry);
      }
      controller.close();
   }
  });
}
globalThis.readableStreamToArray = function(stream) {
  var array = [];
  var writable = new WritableStream({
    write(chunk) {
      array.push(chunk);
    }
  });
  return stream.pipeTo(writable).then(() => array);
}

globalThis.console = {
	debug: print,
	log: print,
	warn: print,
};
globalThis.createBuffer = function(type, length, opts) {
    if (type === "ArrayBuffer")
		return new ArrayBuffer(length, opts);
    if (type === "SharedArrayBuffer")
		return new SharedArrayBuffer(length, opts);
	throw new Error("type has to be ArrayBuffer or SharedArrayBuffer");
}
globalThis.garbageCollect = async () => {
	return $262.gc();
}
globalThis.structuredClone = structuredClone;
globalThis.MessageChannel = class {
	constructor() {
		this.port1 = {
			postMessage(buffer, array) {
				if (buffer instanceof ArrayBuffer)
					return buffer.transfer();
				debugger
			}
		}
	} 
}
import { URL, URLSearchParams } from "url";
globalThis.URL = URL;
globalThis.URLSearchParams = URLSearchParams;

import { } from "self";
import { } from "testharness";
import { } from "test-utils";
import { } from "rs-utils";
import { } from "recording-streams";
import { } from "rs-test-templates";
import { } from "crypto-helpers";
import { } from "subset-tests";

let testCount = 0;
let passedCount = 0;
add_completion_callback((tests, status) => {
	let path = globalThis["<xsbug:path>"];
	path = path.slice(path.indexOf("web/") + 4);
	print(`# ./${path}`);
	for (let test of tests) {
		testCount++;
		if (test.status)
			print(`${test.name}: ${test.message}`);
		else {
// 			print(`${test.name}: OK`);
			passedCount++;
		}
	}
	print(`# ${passedCount}/${testCount}`);
	$DONE();
});
setup({ debug:false });

$MAIN();

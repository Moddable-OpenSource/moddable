/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

import kvp from "embedded:storage/key-value";
import WebStorage from "webstorage";

// bind WebStorage to a domain "localStorage" in embedded:storage/key-value
const localStorage = new WebStorage(kvp.open({path: "localStorage"}));

localStorage.setItem(1, "one");
localStorage.setItem(2, "two");
localStorage.setItem(3, "three");

trace(`length: ${localStorage.length}\n`);
trace(`key(1.2): ${localStorage.key(1.2)}\n`);
trace(`getItem(3): ${localStorage.getItem(3)}\n`);

localStorage.clear();

trace(`length: ${localStorage.length}\n`);
trace(`key(1.2): ${localStorage.key(1.2)}\n`);
trace(`getItem(3): ${localStorage.getItem(3)}\n`);

/*

localStorage persists across sessions, so the storage/key-value instance can be used as-is.

For sessionStorage, the data does not persist. There's no way to ensure the storage is cleared on exit (a crash would prevent this),
	so clear it on creation instead:

let store = kvp.open({path: "sessionStorage"}
store.clear();
globalThis.sessionStorage = new WebStorage(store);

*/

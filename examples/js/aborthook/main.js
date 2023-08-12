/*
 * Copyright (c) 2023  Moddable Tech, Inc.
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

/*
	Note: the abort hook is disabled by default. To enable it
		set the XS abortHook define in the manifest:
		
		"defines": {
			"xs": {
				"abortHook": 1
			}
		}

	The abort hooks is provided for debugging purposes. As a rule it should not
		be used in production as unhandled exceptions and unhandled rejections may
		leave your script code in an unstable state.

	The abort hook is not called on stack overflows because the execution state is not safe.
	
	The abort hook is called on memory full errors. This may work, depending on the cause of the memory
		full error. It does have the potential to fail, so you should not rely on the abort hook
		as a way to recover from memory exhaustion.

	A rejected promise can only be considered unhandled when there are no references to it. This means that
		the abort hook is not be called immediately, but only after at least one garbage collection occurs.
		See this issue on GitHub for more details:
			https://github.com/Moddable-OpenSource/moddable/issues/1186

	You can configure xsbug to NOT stop when an exception is thrown. In Settings,
		turn off Break - On Exceptions.
*/

import Timer from "timer";

globalThis.abort = function (msg, exception) {
	trace(`abort hook called with "${msg}"\n`);
	trace(exception.stack, "\n");

	if (("unhandled exception" === msg) ||
		("unhandled rejection" === msg))
		return false;		// do not abort
}

Timer.set(() => {
	throw new Error("exception from timer");
}, 100);

Timer.repeat(() => {
	trace(Date(), "\n");
}, 1000)

Promise.reject(new Error("exception from promise"));

throw new Error("exception from module top level");

trace("will never reach here\n");

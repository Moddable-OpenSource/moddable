/*
 * Copyright (c) 2020-2026 Moddable Tech, Inc
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

/// <reference path="./web/fetch.d.ts" />
/// <reference path="./web/headers.d.ts" />
/// <reference path="./web/structuredClone.d.ts" />
/// <reference path="./web/url.d.ts" />
/// <reference path="./web/worker.d.ts" />
/// <reference path="./web/websocket.d.ts" />
/// <reference path="./web/webstorage.d.ts" />
/// <reference path="./text/decoder.d.ts" />
/// <reference path="./text/encoder.d.ts" />

import type Timer from "timer";

interface Console {
	log(...log: (string | number | boolean)[]):void;
}

declare global {
	const console: Console;
	const clearImmediate: (timer: Timer) => void;
	const clearInterval: (timer: Timer) => void;
	const clearTimeout: (timer: Timer) => void;
	const setImmediate: (handler: Function) => Timer;
	const setInterval: (handler: Function, timeout?: number) => Timer;
	const setTimeout: (handler: Function, timeout?: number) => Timer;

	const localStorage: import("webstorage").default;

	const WebSocket: typeof import("web/websocket").default;
	type WebSocket = import("web/websocket").default;

	const Headers: typeof import("headers").default;
	type Headers = import("headers").default;

	const fetch: typeof import("fetch").fetch;

	const structuredClone: typeof import("structuredClone").default;

	const TextDecoder: typeof import("text/decoder").default;
	type TextDecoder = import("text/decoder").default;

	const TextEncoder: typeof import("text/encoder").default;
	type TextEncoder = import("text/encoder").default;

	const URL: typeof import("url").URL;
	type URL = import("url").URL;

	const URLSearchParams: typeof import("url").URLSearchParams;
	type URLSearchParams = import("url").URLSearchParams;

	const Worker: typeof import("worker").default;
	type Worker = import("worker").default;

	const SharedWorker: typeof import("worker").SharedWorker;
	type SharedWorker = import("worker").SharedWorker;
}


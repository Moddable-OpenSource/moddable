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

/// <reference path="./host/index.d.ts" />
/// <reference path="./web/abortsignal.d.ts" />
/// <reference path="./web/console.d.ts" />
/// <reference path="./web/decompressionstream.d.ts" />
/// <reference path="./web/domexception.d.ts" />
/// <reference path="./web/eventsource.d.ts" />
/// <reference path="./web/fetch.d.ts" />
/// <reference path="./web/headers.d.ts" />
/// <reference path="./web/streams.d.ts" />
/// <reference path="./web/structuredClone.d.ts" />
/// <reference path="./web/textdecoderstream.d.ts" />
/// <reference path="./web/textencoderstream.d.ts" />
/// <reference path="./web/timers.d.ts" />
/// <reference path="./web/url.d.ts" />
/// <reference path="./web/websocket.d.ts" />
/// <reference path="./web/websocketstream.d.ts" />
/// <reference path="./web/webstorage.d.ts" />
/// <reference path="./web/worker.d.ts" />
/// <reference path="./text/decoder.d.ts" />
/// <reference path="./text/encoder.d.ts" />

export {};

declare global {
	const EventSource: typeof import("web/eventsource").default;
	type EventSource = import("web/eventsource").default;

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

	const AbortController: typeof import("web/abortsignal").AbortController;
	type AbortController = import("web/abortsignal").AbortController;

	const AbortSignal: typeof import("web/abortsignal").AbortSignal;
	type AbortSignal = import("web/abortsignal").AbortSignal;

	const DOMException: typeof import("web/domexception").DOMException;
	type DOMException = import("web/domexception").DOMException;

	const ReadableStream: typeof import("web/streams").ReadableStream;
	type ReadableStream<R = any> = import("web/streams").ReadableStream<R>;

	const ReadableStreamDefaultReader: typeof import("web/streams").ReadableStreamDefaultReader;
	type ReadableStreamDefaultReader<R = any> = import("web/streams").ReadableStreamDefaultReader<R>;

	const ReadableStreamBYOBReader: typeof import("web/streams").ReadableStreamBYOBReader;
	type ReadableStreamBYOBReader = import("web/streams").ReadableStreamBYOBReader;

	const ReadableStreamDefaultController: typeof import("web/streams").ReadableStreamDefaultController;
	type ReadableStreamDefaultController<R = any> = import("web/streams").ReadableStreamDefaultController<R>;

	const ReadableByteStreamController: typeof import("web/streams").ReadableByteStreamController;
	type ReadableByteStreamController = import("web/streams").ReadableByteStreamController;

	const ReadableStreamBYOBRequest: typeof import("web/streams").ReadableStreamBYOBRequest;
	type ReadableStreamBYOBRequest = import("web/streams").ReadableStreamBYOBRequest;

	const WritableStream: typeof import("web/streams").WritableStream;
	type WritableStream<W = any> = import("web/streams").WritableStream<W>;

	const WritableStreamDefaultWriter: typeof import("web/streams").WritableStreamDefaultWriter;
	type WritableStreamDefaultWriter<W = any> = import("web/streams").WritableStreamDefaultWriter<W>;

	const WritableStreamDefaultController: typeof import("web/streams").WritableStreamDefaultController;
	type WritableStreamDefaultController = import("web/streams").WritableStreamDefaultController;

	const TransformStream: typeof import("web/streams").TransformStream;
	type TransformStream<I = any, O = any> = import("web/streams").TransformStream<I, O>;

	const TransformStreamDefaultController: typeof import("web/streams").TransformStreamDefaultController;
	type TransformStreamDefaultController<O = any> = import("web/streams").TransformStreamDefaultController<O>;

	const ByteLengthQueuingStrategy: typeof import("web/streams").ByteLengthQueuingStrategy;
	type ByteLengthQueuingStrategy = import("web/streams").ByteLengthQueuingStrategy;

	const CountQueuingStrategy: typeof import("web/streams").CountQueuingStrategy;
	type CountQueuingStrategy = import("web/streams").CountQueuingStrategy;

	const DecompressionStream: typeof import("web/decompressionstream").default;
	type DecompressionStream = import("web/decompressionstream").default;

	const TextDecoderStream: typeof import("web/textdecoderstream").default;
	type TextDecoderStream = import("web/textdecoderstream").default;

	const TextEncoderStream: typeof import("web/textencoderstream").default;
	type TextEncoderStream = import("web/textencoderstream").default;

	const WebSocketStream: typeof import("web/websocketstream").default;
	type WebSocketStream = import("web/websocketstream").default;
}


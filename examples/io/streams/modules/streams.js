// specification: https://streams.spec.whatwg.org
// reference implementation: https://github.com/whatwg/streams

function queueMicrotask(f) {
	Promise.resolve(undefined).then(result => {
		f();
	});
};

function assert(it) {
	if (!it) {
		debugger;
		throw new TypeError("assertion");
	}
}
function ignoreReason(reason) {
// 	trace(`ignore ${ reason }\n`);
}
function verbose(it) {
// 	trace(`${ it }\n`);
}

class Brand {
	constructor(name) {
		this.name = name;
		this.token = Symbol(name);
		return Object.freeze(this);
	}
	get(it, name = "this") {
		let internal = it?.[this.token];
		if (internal)
			return internal;
		throw new TypeError(`${name} is no ${this.name}`);
	}
	has(it) {
		let internal = it?.[this.token];
		return internal !== undefined;
	}
	set(it, internal) {
		it[this.token] = internal;
	}
}

class DOMException extends Error {
	constructor(message, name) {
		super(message);
		Object.defineProperty(this, "name", { value:name});
	}
}

const AbortSignalBrand = new Brand("AbortSignal");
class AbortSignal {
	constructor(token) {
		if (token !== AbortSignalBrand.token)
			throw new TypeError("new AbortSignal");
		const signal = {
			self: this,
			aborted: false,
			dependents: null,
			eventListeners: { abort:[] },
			reason: undefined,
		}
		AbortSignalBrand.set(this, signal);
		return signal;
	}
	get aborted() {
		const signal = AbortSignalBrand.get(this);
		return signal.aborted;
	}
	get reason() {
		const signal = AbortSignalBrand.get(this);
		return signal.reason;
	}
	addEventListener(event, listener) {
		const signal = AbortSignalBrand.get(this);
		let listeners = signal.eventListeners[event];
		if (!listeners)
			throw new Error("no such event");
		listeners.push(listener);
	}
	removeEventListener(event, listener) {
		const signal = AbortSignalBrand.get(this);
		let listeners = signal.eventListeners[event];
		if (!listeners)
			throw new Error("no such event");
		let index = listeners.find(item => item === listener);
		if (index >= 0)
			listeners.splice(index, 1);
	}
	static abort(reason) {
		const signal = new AbortSignal(AbortSignalBrand.token);
		triggerAbortSignal(signal, reason);
		return signal.self;
	}
	static any(iterable) {
		const signal = new AbortSignal(AbortSignalBrand.token);
		const sources = [];
		for (let self of iterable) {
			const source = AbortSignalBrand.get(self);
			if (sources.indexOf(source) < 0)
				source.push(source);
		}
		for (let source of sources) {
			if (source.aborted) {
				triggerAbortSignal(signal, source.reason);
				return signal.self;
			}
		}
		for (let source of sources) {
			if (source.dependents)
				source.dependents.push(signal);
			else
				source.dependents = [ signal ];
		}
		return signal.self;
	}
	static timeout(duration) {
		const signal = new AbortSignal(AbortSignalBrand.token);
		setTimeout(() => {
			triggerAbortSignal(signal, new Error());
		}, duration);
		return signal.self;
	}
}
function triggerAbortSignal(signal, reason) {
	if (signal.aborted)
		return;
	if (reason === undefined) {
		reason = new DOMException("", "AbortError");
		reason.code = 20;
	}
	signal.aborted = true;
	signal.reason = reason;
	const event = {
		signal:signal.self,
	}
	signal.eventListeners.abort.forEach(eventListener => eventListener.call(null, event));
	signal.dependents?.forEach(dependent => triggerAbortSignal(dependent, reason));
	signal.dependents = null;
}

class AbortController {
	#signal;
	constructor() {
		this.#signal = new AbortSignal(AbortSignalBrand.token);
	}
	get signal() {
		return this.#signal.self;
	}
	abort(reason) {
		triggerAbortSignal(this.#signal, reason);
	}
}

const originalFunctionPrototypeApply = Function.prototype.apply;

function createCallback(callback, target) {
	if (callback === undefined)
		return defaultCallback;
	if (typeof(callback) !== "function")
		throw new TypeError("callback is no function");
	return (...args) => originalFunctionPrototypeApply.call(callback, target, args);
}
function defaultCallback() {
}
function createPromiseCallback(callback, target, other=defaultPromiseCallback) {
	if (callback === undefined)
		return other;
	if (typeof(callback) !== "function")
		throw new TypeError("callback is no function");
	return (...args) => {
		try {
			return Promise.resolve(originalFunctionPrototypeApply.call(callback, target, args));
		}
		catch(e) {
			return Promise.reject(e);
		}
	}
}
function defaultPromiseCallback() {
	return Promise.resolve(undefined);
}

function createPromise() {
	const record = {
		pending: true
	};
	record.promise = new Promise((resolve, reject) => {
		record.resolve = resolve;
		record.reject = reject;
	});
	return record;
}
function createRejectedPromise(reason) {
	const record = {
		pending: false
	};
	record.promise = Promise.reject(reason);
	return record;
}
function createResolvedPromise(value) {
	const record = {
		pending: false
	};
	record.promise = Promise.resolve(value);
	return record;
}
function handlePromise(record) {
	record.promise.catch(ignoreReason);
}
function isPromisePending(record) {
	return record.pending;
}
function resolvePromise(record, value) {
	if (record.pending) {
		record.pending = false;
		record.resolve(value);
	}
}
function rejectPromise(record, reason) {
	if (record.pending) {
		record.pending = false;
		record.reject(reason);
	}
}


function CanTransferArrayBuffer(it) {
	return !it.detached;
}
function IsDetachedBuffer(it) {
	return it.detached;
}
function TransferArrayBuffer(it) {
	return it.transfer();
}
function CopyDataBlockBytes(dest, destOffset, src, srcOffset, n) {
	new Uint8Array(dest).set(new Uint8Array(src, srcOffset, n), destOffset);
}
function CloneAsUint8Array(it) {
	const buffer = it.buffer.slice(it.byteOffset, it.byteOffset + it.byteLength);
	return new Uint8Array(buffer);
}


// 4.2 ReadableStream
const ReadableStreamBrand = new Brand("ReadableStream");
class ReadableStream {
	constructor(underlyingSource, strategy) {
		const stream = {
			self:this
		};
		ReadableStreamBrand.set(this, stream);
		if (underlyingSource == ReadableStreamBrand.token)
			return stream;
		if (underlyingSource === null)
			throw new TypeError("invalid underlyingSource");
		const underlyingSourceDict = underlyingSource ?? {};
		InitializeReadableStream(stream);
		if (underlyingSourceDict.type && (String(underlyingSourceDict.type) == "bytes")) {
			if (strategy && ("size" in strategy))
				throw new RangeError('The strategy for a byte stream cannot have a size function');
			const highWaterMark = ExtractHighWaterMark(strategy, 0);
			SetUpReadableByteStreamControllerFromUnderlyingSource(stream, underlyingSource, underlyingSourceDict, highWaterMark);
		}
		else {
			if (underlyingSourceDict.type !== undefined)
				throw new TypeError("invalid underlyingSource type");
			const sizeAlgorithm = ExtractSizeAlgorithm(strategy);
			const highWaterMark = ExtractHighWaterMark(strategy, 1);
			SetUpReadableStreamDefaultControllerFromUnderlyingSource(stream, underlyingSource, underlyingSourceDict, highWaterMark, sizeAlgorithm);
		}
	}
	get locked() {
		const stream = ReadableStreamBrand.get(this);
		return IsReadableStreamLocked(stream);
	}
	cancel(reason) {
		const stream = ReadableStreamBrand.get(this);
		if (IsReadableStreamLocked(stream))
			return Promise.reject(new TypeError("stream locked"));
		return ReadableStreamCancel(stream, reason);
	}
	getReader(options) {
		const stream = ReadableStreamBrand.get(this);
		if (!options || !("mode" in options) || (options.mode === undefined))
			return new ReadableStreamDefaultReader(this);
		if (String(options.mode) !== "byob")
			throw new TypeError("invalid mode");
		return new ReadableStreamBYOBReader(this);
	}
	pipeThrough(transform, options) {
		const readableStream = ReadableStreamBrand.get(this);
		const checkStream = ReadableStreamBrand.get(transform.readable, "transform.readable");
		const writableStream = WritableStreamBrand.get(transform.writable, "transform.writable");
		const preventAbort = Boolean(options?.preventAbort);
		const preventCancel = Boolean(options?.preventCancel);
		const preventClose = Boolean(options?.preventClose);
		const signal = options ? options.signal : undefined;
		if (IsReadableStreamLocked(readableStream))
			throw new TypeError("stream locked");
		if (IsWritableStreamLocked(writableStream))
			throw new TypeError("transform.writable stream locked");
		let promise = ReadableStreamPipeTo(readableStream, writableStream, preventClose, preventAbort, preventCancel, signal);
		promise.catch(ignoreReason);
		return transform.readable;
	}
	pipeTo(destination, options) {
		try {
			const readableStream = ReadableStreamBrand.get(this);
			const writableStream = WritableStreamBrand.get(destination, "destination");
			const preventAbort = Boolean(options?.preventAbort);
			const preventCancel = Boolean(options?.preventCancel);
			const preventClose = Boolean(options?.preventClose);
			const signal = options?.signal;
			if (IsReadableStreamLocked(readableStream))
				throw new TypeError("stream locked");
			if (IsWritableStreamLocked(writableStream))
				throw new TypeError("destination stream locked");
			return ReadableStreamPipeTo(readableStream, writableStream, preventClose, preventAbort, preventCancel, signal);
		}
		catch(error) {
			return Promise.reject(error);
		}
	}
	tee() {
		const stream = ReadableStreamBrand.get(this);
		const result = ReadableStreamTee(stream, false);
		result[0] = result[0].self;
		result[1] = result[1].self;
		return result;
	}
	values(options) {
		const stream = ReadableStreamBrand.get(this);
		const preventCancel = Boolean(options?.preventCancel);
		return createReadableStreamAsyncIterator(stream, preventCancel);
	}
	[Symbol.asyncIterator]() {
		const stream = ReadableStreamBrand.get(this);
		return createReadableStreamAsyncIterator(stream, false);
	}
	static from(asyncIterable) {
		return ReadableStreamFromIterable(asyncIterable).self;
	}
}
// 4.2.5 Asynchronous iteration
const ReadableStreamAsyncIteratorBrand = new Brand("ReadableStreamAsyncIterator");
const ReadableStreamAsyncIteratorPrototype = Object.create(Object.getPrototypeOf(Object.getPrototypeOf(async function* () {}).prototype), {
	[Symbol.toStringTag]: {
		value: "ReadableStreamAsyncIterator",
		configurable: true
	}
});
Object.assign(ReadableStreamAsyncIteratorPrototype, {
	next() {
		const internal = ReadableStreamAsyncIteratorBrand.get(this);
		const reader = internal.reader;
		const nextSteps = () => {
			if (internal.isFinished)
				return Promise.resolve({ value:undefined, done:true });
			assert(reader.stream !== undefined);
			const promiseRecord = createPromise();
			const readRequest = {
				chunkSteps: chunk => resolvePromise(promiseRecord, chunk),
				closeSteps: () => {
					ReadableStreamDefaultReaderRelease(reader);
					resolvePromise(promiseRecord, ReadableStreamAsyncIteratorBrand.token);
			  	},
				errorSteps: e => {
					ReadableStreamDefaultReaderRelease(reader);
					rejectPromise(promiseRecord, e);
				}
			};
			ReadableStreamDefaultReaderRead(reader, readRequest);
			return promiseRecord.promise.then(
				value => {
					internal.ongoingPromise = null;
					if (value === ReadableStreamAsyncIteratorBrand.token) {
						internal.isFinished = true;
						return { value: undefined, done: true };
					}
					return { value, done: false };
				},
				reason => {
					internal.ongoingPromise = null;
					internal.isFinished = true;
					throw reason;
				}
			);
		};
		internal.ongoingPromise = internal.ongoingPromise
			? internal.ongoingPromise.then(nextSteps, nextSteps)
			: nextSteps();
		return internal.ongoingPromise;
	},
	return(value) {
		const internal = ReadableStreamAsyncIteratorBrand.get(this);
		const reader = internal.reader;
		const returnSteps = () => {
			if (internal.isFinished) {
				return Promise.resolve({ value, done: true });
			}
			internal.isFinished = true;
			assert(reader.stream !== undefined);
			assert(reader.readRequests.length === 0);
			if (internal.preventCancel === false) {
			  const result = ReadableStreamReaderGenericCancel(reader, value);
			  ReadableStreamDefaultReaderRelease(reader);
			  return result;
			}
			ReadableStreamDefaultReaderRelease(reader);
			return Promise.resolve(undefined);
		};
		const returnPromise = internal.ongoingPromise
			? internal.ongoingPromise.then(returnSteps, returnSteps)
			: returnSteps();
		return returnPromise.then(() => ({ value, done: true }));
	}
});
Object.freeze(ReadableStreamAsyncIteratorPrototype);
function createReadableStreamAsyncIterator(stream, preventCancel) {
	const reader = AcquireReadableStreamDefaultReader(stream);
	const iterator = Object.create(ReadableStreamAsyncIteratorPrototype);
	const internal = { reader, preventCancel, ongoingPromise: null, isFinished: false };
	ReadableStreamAsyncIteratorBrand.set(iterator, internal);
	return iterator;
};

// 4.4 ReadableStreamDefaultReader
const ReadableStreamDefaultReaderBrand = new Brand("ReadableStreamDefaultReader");
class ReadableStreamDefaultReader {
	constructor(stream, token) {
		const reader = { self:this }
		ReadableStreamDefaultReaderBrand.set(this, reader);
		stream = ReadableStreamBrand.get(stream, "stream");
		SetUpReadableStreamDefaultReader(reader, stream);
		if (token === ReadableStreamDefaultReaderBrand.token)
			return reader;
	}
	get closed() {
		const reader = ReadableStreamDefaultReaderBrand.get(this);
		return reader.closedPromise.promise;
	}
	cancel(reason) {
		const reader = ReadableStreamDefaultReaderBrand.get(this);
		const stream = reader.stream;
		if (stream === undefined)
			return Promise.reject(new TypeError("no stream"));
		return ReadableStreamReaderGenericCancel(reader, reason);
	}
	read() {
		const reader = ReadableStreamDefaultReaderBrand.get(this);
		const stream = reader.stream;
		if (stream === undefined)
			return Promise.reject(new TypeError("no stream"));
		const promiseRecord = createPromise();
		const readRequest = {
			chunkSteps: chunk => resolvePromise(promiseRecord, { value: chunk, done: false }),
			closeSteps: () => resolvePromise(promiseRecord, { value: undefined, done: true }),
			errorSteps: e => rejectPromise(promiseRecord, e)
		};
		ReadableStreamDefaultReaderRead(reader, readRequest);
		return promiseRecord.promise;
	}
	releaseLock() {
		const reader = ReadableStreamDefaultReaderBrand.get(this);
		const stream = reader.stream;
		if (stream === undefined)
			return;
		ReadableStreamDefaultReaderRelease(reader);
	}
}

// 4.5 ReadableStreamBYOBReader
const ReadableStreamBYOBReaderBrand = new Brand("ReadableStreamBYOBReader");
class ReadableStreamBYOBReader {
	constructor(stream, token) {
		const reader = { self:this }
		ReadableStreamBYOBReaderBrand.set(this, reader);
		stream = ReadableStreamBrand.get(stream, "stream");
		SetUpReadableStreamBYOBReader(reader, stream);
		if (token === ReadableStreamBYOBReaderBrand.token)
			return reader;
	}
	get closed() {
		const reader = ReadableStreamBYOBReaderBrand.get(this);
		return reader.closedPromise.promise;
	}
	cancel(reason) {
		const reader = ReadableStreamBYOBReaderBrand.get(this);
		const stream = reader.stream;
		if (stream === undefined)
			return Promise.reject(new TypeError("no stream"));
		return ReadableStreamReaderGenericCancel(reader, reason);
	}
	read(view, options) {
		const reader = ReadableStreamBYOBReaderBrand.get(this);
		const stream = reader.stream;
		if (stream === undefined)
			return Promise.reject(new TypeError("no stream"));
		if (!ArrayBuffer.isView(view))
			return Promise.reject(new TypeError("no view"));
		if (view.byteLength == 0)
			return Promise.reject(new TypeError("zero view byteLength"));
		if (view.buffer.byteLength == 0)
			return Promise.reject(new TypeError("zero buffer byteLength"));
		if (view.buffer.detached)
			return Promise.reject(new TypeError("detached buffer"));
		let min = 1;	
		if (options && ("min" in options))
			min = options.min;
		if (min <= 0)
			return Promise.reject(new TypeError("min <= 0"));
		if (view.constructor !== DataView) {
			if (min > view.length) {
				return Promise.reject(new RangeError("options.min > view.length"));
			}
		}
		else if (options.min > view.byteLength)
			return Promise.reject(new RangeError("options.min > view.byteLength"));
			
		const promiseRecord = createPromise();
		const readIntoRequest = {
			chunkSteps: chunk => resolvePromise(promiseRecord, { value: chunk, done: false }),
			closeSteps: chunk => resolvePromise(promiseRecord, { value: chunk, done: true }),
			errorSteps: e => rejectPromise(promiseRecord, e)
		};
		ReadableStreamBYOBReaderRead(reader, view, min, readIntoRequest);
		return promiseRecord.promise;
	}
	releaseLock() {
		const reader = ReadableStreamBYOBReaderBrand.get(this);
		const stream = reader.stream;
		if (stream === undefined)
			return;
		ReadableStreamBYOBReaderRelease(reader);
	}
}

// 4.6 ReadableStreamDefaultController
const ReadableStreamDefaultControllerBrand = new Brand("ReadableStreamDefaultController");
class ReadableStreamDefaultController {
	constructor(token) {
		if (token !== ReadableStreamDefaultControllerBrand.token)
			throw new TypeError("new ReadableStreamDefaultController");
		const controller = { 
			self: this,
			cancelSteps: ReadableStreamDefaultControllerCancelSteps,
			pullSteps: ReadableStreamDefaultControllerPullSteps,
			releaseSteps: ReadableStreamDefaultControllerReleaseSteps,
		};
		ReadableStreamDefaultControllerBrand.set(this, controller);
		return controller;
	}
	get desiredSize() {
		const controller = ReadableStreamDefaultControllerBrand.get(this);
		return ReadableStreamDefaultControllerGetDesiredSize(controller);
	}
	close() {
		const controller = ReadableStreamDefaultControllerBrand.get(this);
		if (!ReadableStreamDefaultControllerCanCloseOrEnqueue(controller))
			throw new TypeError("cannot close");
		ReadableStreamDefaultControllerClose(controller);
	}
	enqueue(chunk) {
		const controller = ReadableStreamDefaultControllerBrand.get(this);
		if (!ReadableStreamDefaultControllerCanCloseOrEnqueue(controller))
			throw new TypeError("cannot enqueue");
		ReadableStreamDefaultControllerEnqueue(controller, chunk);
	}
	error(e) {
		const controller = ReadableStreamDefaultControllerBrand.get(this);
		ReadableStreamDefaultControllerError(controller, e);
	}
}
// 4.6.4 Internal methods
function ReadableStreamDefaultControllerCancelSteps(reason) {
	ResetQueue(this);
	let result = this.cancelAlgorithm(reason);
	ReadableStreamDefaultControllerClearAlgorithms(this);
	return result;
}
function ReadableStreamDefaultControllerPullSteps(readRequest) {
	let stream = this.stream;
	if (this.queue.length > 0) {
		let chunk = DequeueValue(this);
		if (this.closeRequested && (this.queue.length == 0)) {
			ReadableStreamDefaultControllerClearAlgorithms(this);
			ReadableStreamClose(stream);
		}
		else
			ReadableStreamDefaultControllerCallPullIfNeeded(this);
		readRequest.chunkSteps(chunk);
	}
	else {
		ReadableStreamAddReadRequest(stream, readRequest);
		ReadableStreamDefaultControllerCallPullIfNeeded(this);
	}
}
function ReadableStreamDefaultControllerReleaseSteps() {
}

// 4.7 ReadableByteStreamController
const ReadableByteStreamControllerBrand = new Brand("ReadableByteStreamController");
class ReadableByteStreamController {
	constructor(token) {
		if (token !== ReadableByteStreamControllerBrand.token)
			throw new TypeError("new ReadableByteStreamController");
		const controller = { 
			self: this,
			cancelSteps: ReadableByteStreamControllerCancelSteps,
			pullSteps: ReadableByteStreamControllerPullSteps,
			releaseSteps: ReadableByteStreamControllerReleaseSteps,
		};
		ReadableByteStreamControllerBrand.set(this, controller);
		return controller;
	}
	get byobRequest() {
		const controller = ReadableByteStreamControllerBrand.get(this);
		const byobRequest = ReadableByteStreamControllerGetBYOBRequest(controller);
		return (byobRequest) ? byobRequest.self : null;
	}
	get desiredSize() {
		const controller = ReadableByteStreamControllerBrand.get(this);
		return ReadableByteStreamControllerGetDesiredSize(controller);
	}
	close() {
		const controller = ReadableByteStreamControllerBrand.get(this);
		if (controller.closeRequested)
			throw new TypeError("closing");
		if (controller.stream.state != "readable")
			throw new TypeError(controller.stream.state);
		ReadableByteStreamControllerClose(controller);
	}
	enqueue(chunk) {
		const controller = ReadableByteStreamControllerBrand.get(this);
		if (chunk.byteLength == 0)
			throw new TypeError("no bytes");
		if (controller.closeRequested)
			throw new TypeError("closing");
		if (controller.stream.state != "readable")
			throw new TypeError(controller.stream.state);
		ReadableByteStreamControllerEnqueue(controller, chunk);
	}
	error(e) {
		const controller = ReadableByteStreamControllerBrand.get(this);
		ReadableByteStreamControllerError(controller, e);
	}
}
// 4.7.4 Internal methods
function ReadableByteStreamControllerCancelSteps(reason) {
	ReadableByteStreamControllerClearPendingPullIntos(this);
	ResetQueue(this);
	let result = this.cancelAlgorithm(reason);
	ReadableByteStreamControllerClearAlgorithms(this);
	return result;
}
function ReadableByteStreamControllerPullSteps(readRequest) {
	let stream = this.stream;
	assert(ReadableStreamHasDefaultReader(stream));
	if (this.queueTotalSize > 0) {
		assert(ReadableStreamGetNumReadRequests(stream) === 0);
		ReadableByteStreamControllerFillReadRequestFromQueue(this, readRequest);
		return;
	}
	let autoAllocateChunkSize = this.autoAllocateChunkSize;
	if (autoAllocateChunkSize !== undefined) {
		let buffer;
		try {
			buffer = new ArrayBuffer(autoAllocateChunkSize);
		}
		catch(e) {
			readRequest.errorSteps(e);
			return;
		}
		let pullInfoDescriptor = {
			buffer, 
			bufferByteLength:autoAllocateChunkSize, 
			byteOffset:0, 
			byteLength:autoAllocateChunkSize, 
			bytesFilled:0, 
			minimumFill:1, 
			elementSize:1, 
			ViewConstructor:Uint8Array, 
			readerType:"default"
		};
		this.pendingPullIntos.push(pullInfoDescriptor);
	}
	ReadableStreamAddReadRequest(stream, readRequest);
	ReadableByteStreamControllerCallPullIfNeeded(this);
}
function ReadableByteStreamControllerReleaseSteps() {
	if (this.pendingPullIntos.length > 0) {
		this.pendingPullIntos[0].readerType = "none";
	}
}

// 4.8 ReadableStreamBYOBRequest 
const ReadableStreamBYOBRequestBrand = new Brand("ReadableStreamBYOBRequest");
class ReadableStreamBYOBRequest {
	constructor(token) {
		if (token !== ReadableStreamBYOBRequestBrand.token)
			throw new TypeError("new ReadableStreamBYOBRequest");
		const byobRequest = {
			self: this,
		};
		ReadableStreamBYOBRequestBrand.set(this, byobRequest);
		return byobRequest;
	}
	get view() {
		const byobRequest = ReadableStreamBYOBRequestBrand.get(this);
		return byobRequest.view;
	}
	respond(bytesWritten) {
		const byobRequest = ReadableStreamBYOBRequestBrand.get(this);
		const controller = byobRequest.controller;
		if (controller === undefined)
			throw new TypeError("no controller");
		if (byobRequest.view.byteLength == 0)
			throw new TypeError("no buffer");
		ReadableByteStreamControllerRespond(controller, bytesWritten);
	}
	respondWithNewView(view) {
		const byobRequest = ReadableStreamBYOBRequestBrand.get(this);
		const controller = byobRequest.controller;
		if (controller === undefined)
			throw new TypeError("no controller");
		if (view.buffer.detached)
			throw new TypeError("detached buffer");
		ReadableByteStreamControllerRespondWithNewView(controller, view);
	}
}

// 4.9.1 Working with readable streams 
const defaultSizeAlgorithm = function() { return 1 }

function CreateReadableStream(startAlgorithm, pullAlgorithm, cancelAlgorithm, highWaterMark = 1, sizeAlgorithm = () => 1) {
	assert(IsNonNegativeNumber(highWaterMark) === true);
	const stream = new ReadableStream(ReadableStreamBrand.token);
	InitializeReadableStream(stream);
	const controller = new ReadableStreamDefaultController(ReadableStreamDefaultControllerBrand.token);
	SetUpReadableStreamDefaultController(
		stream, controller, startAlgorithm, pullAlgorithm, cancelAlgorithm, highWaterMark, sizeAlgorithm
	);
	return stream;
}
function CreateReadableByteStream(startAlgorithm, pullAlgorithm, cancelAlgorithm) {
	const stream = new ReadableStream(ReadableStreamBrand.token);
	InitializeReadableStream(stream);
	const controller = new ReadableByteStreamController(ReadableByteStreamControllerBrand.token);
	SetUpReadableByteStreamController(
		stream, controller, startAlgorithm, pullAlgorithm, cancelAlgorithm, 0, undefined
	);
	return stream;
}
function AcquireReadableStreamBYOBReader(stream) {
	return new ReadableStreamBYOBReader(stream.self, ReadableStreamBYOBReaderBrand.token);
}
function AcquireReadableStreamDefaultReader(stream) {
	return new ReadableStreamDefaultReader(stream.self, ReadableStreamDefaultReaderBrand.token);
}
function InitializeReadableStream(stream) {
	stream.disturbed = false;
	stream.reader = undefined;
	stream.state = "readable";
	stream.storedError = undefined;
}
function IsReadableStreamLocked(stream) {
	return stream.reader !== undefined;
}
function ReadableStreamPipeTo(source, dest, preventClose, preventAbort, preventCancel, signal) {
	assert(ReadableStreamBrand.has(source.self));
	assert(WritableStreamBrand.has(dest.self));
	assert(typeof preventClose === 'boolean');
	assert(typeof preventAbort === 'boolean');
	assert(typeof preventCancel === 'boolean');
	if (signal !== undefined)
		signal = AbortSignalBrand.get(signal);
	assert(IsReadableStreamLocked(source) === false);
	assert(IsWritableStreamLocked(dest) === false);
	const reader = AcquireReadableStreamDefaultReader(source);
	const writer = AcquireWritableStreamDefaultWriter(dest);
	source.disturbed = true;
	let shuttingDown = false;
	// This is used to keep track of the spec's requirement that we wait for ongoing writes during shutdown.
	let currentWrite = createResolvedPromise(undefined);
	return new Promise((resolve, reject) => {
		let abortAlgorithm;
		if (signal !== undefined) {
			abortAlgorithm = () => {
				const error = signal.reason;
				const actions = [];
				if (preventAbort === false) {
					actions.push(() => {
						if (dest.state === 'writable') {
							return WritableStreamAbort(dest, error);
						}
						return Promise.resolve(undefined);
					});
				}
				if (preventCancel === false) {
					actions.push(() => {
						if (source.state === 'readable') {
							return ReadableStreamCancel(source, error);
						}
						return Promise.resolve(undefined);
					});
				}
				shutdownWithAction(() => Promise.all(actions.map(action => action())), true, error);
			};
			if (signal.aborted === true) {
				abortAlgorithm();
				return;
			}
			signal.self.addEventListener('abort', abortAlgorithm);
		}

		// Using reader and writer, read all chunks from this and write them to dest
		// - Backpressure must be enforced
		// - Shutdown must stop all activity
		function pipeLoop() {
			return new Promise((resolveLoop, rejectLoop) => {
				function next(done) {
					if (done) {
						resolveLoop();
					} else {
						pipeStep().then(next, rejectLoop);
					}
				}

				next(false);
			});
		}

		function pipeStep() {
			if (shuttingDown === true) {
				return Promise.resolve(true);
			}

			return writer.readyPromise.promise.then(() => {
				return new Promise((resolveRead, rejectRead) => {
					ReadableStreamDefaultReaderRead(
						reader,
						{
							chunkSteps: chunk => {
								currentWrite = createPromise();
								WritableStreamDefaultWriterWrite(writer,chunk).then(
									() => {
										resolvePromise(currentWrite, undefined);
									},
									(e) => { 
										resolvePromise(currentWrite, undefined);
									}
								);
								resolveRead(false);
							},
							closeSteps: () => resolveRead(true),
							errorSteps: rejectRead
						}
					);
				});
			});
		}

		// Errors must be propagated forward
		isOrBecomesErrored(source, reader.closedPromise.promise, storedError => {
			if (preventAbort === false) {
				shutdownWithAction(() => WritableStreamAbort(dest, storedError), true, storedError);
			} else {
				shutdown(true, storedError);
			}
		});

		// Errors must be propagated backward
		isOrBecomesErrored(dest, writer.closedPromise.promise, storedError => {
			if (preventCancel === false) {
				shutdownWithAction(() => ReadableStreamCancel(source, storedError), true, storedError);
			} else {
				shutdown(true, storedError);
			}
		});

		// Closing must be propagated forward
		isOrBecomesClosed(source, reader.closedPromise.promise, () => {
			if (preventClose === false) {
				shutdownWithAction(() => WritableStreamDefaultWriterCloseWithErrorPropagation(writer));
			} else {
				shutdown();
			}
		});

		// Closing must be propagated backward
		if (WritableStreamCloseQueuedOrInFlight(dest) === true || dest.state === 'closed') {
			const destClosed = new TypeError('the destination writable stream closed before all data could be piped to it');

			if (preventCancel === false) {
				shutdownWithAction(() => ReadableStreamCancel(source, destClosed), true, destClosed);
			} else {
				shutdown(true, destClosed);
			}
		}

		pipeLoop().then(() => undefined).catch(ignoreReason);

		function waitForWritesToFinish() {
			// Another write may have started while we were waiting on this currentWrite, so we have to be sure to wait
			// for that too.
			const oldCurrentWrite = currentWrite;
			return currentWrite.promise.then(
				() => oldCurrentWrite !== currentWrite ? waitForWritesToFinish() : undefined
			);
		}

		function isOrBecomesErrored(stream, promise, action) {
			if (stream.state === 'errored') {
				action(stream.storedError);
			} else {
				promise.catch(action);
			}
		}

		function isOrBecomesClosed(stream, promise, action) {
			if (stream.state === 'closed') {
				action();
			} else {
				promise.then(action).catch(ignoreReason);
			}
		}

		function shutdownWithAction(action, originalIsError, originalError) {
			if (shuttingDown === true) {
				return;
			}
			shuttingDown = true;

			if (dest.state === 'writable' && WritableStreamCloseQueuedOrInFlight(dest) === false) {
				waitForWritesToFinish().then(doTheRest).catch(ignoreReason);
			}
			else {
				doTheRest();
			}

			function doTheRest() {
				action().then(
					() => finalize(originalIsError, originalError),
					newError => finalize(true, newError)
				);
			}
		}

		function shutdown(isError, error) {
			if (shuttingDown === true) {
				return;
			}
			shuttingDown = true;

			if (dest.state === 'writable' && WritableStreamCloseQueuedOrInFlight(dest) === false) {
				waitForWritesToFinish().then(() => finalize(isError, error));
			}
			else {
				finalize(isError, error);
			}
		}
		function finalize(isError, error) {
			WritableStreamDefaultWriterRelease(writer);
			ReadableStreamDefaultReaderRelease(reader);

			if (signal !== undefined) {
				signal.self.removeEventListener('abort', abortAlgorithm);
			}
			if (isError) {
				reject(error);
			}
			else {
				resolve(undefined);
			}
		}
	});
}
function ReadableStreamTee(stream, cloneForBranch2) {
	assert(ReadableStreamBrand.has(stream.self));
	assert(typeof(cloneForBranch2) == "boolean");
	const controller = stream.controller;
	if (ReadableByteStreamControllerBrand.has(controller.self)) 
		return ReadableByteStreamTee(stream);
	return ReadableStreamDefaultTee(stream, cloneForBranch2);
}
function ReadableStreamDefaultTee(stream) {
	assert(ReadableStreamBrand.has(stream.self));
	const reader = AcquireReadableStreamDefaultReader(stream);
	let reading = false;
	let readAgain = false;
	let canceled1 = false;
	let canceled2 = false;
	let reason1;
	let reason2;
	let branch1;
	let branch2;
	let cancelPromise = createPromise();
	function pullAlgorithm() {
		if (reading) {
			readAgain = true;
			return Promise.resolve(undefined);
		}
		reading = true;
		const readRequest = {
			chunkSteps: chunk => {
				queueMicrotask(() => {
					readAgain = false;
					const chunk1 = chunk;
					const chunk2 = chunk;
					// if (canceled2 === false && cloneForBranch2 === true) {
					//	 try {
					//		 chunk2 = StructuredClone(chunk2);
					//	 }
					//	 catch (e) {
					//		 ReadableByteStreamControllerError(branch1.controller, e);
					//		 ReadableByteStreamControllerError(branch2.controller, e);
					//		 resolvePromise(cancelPromise, ReadableStreamCancel(stream, e));
					//		 return;
					//	 }
					// }
					if (canceled1 === false) {
						ReadableStreamDefaultControllerEnqueue(branch1.controller, chunk1);
					}
					if (canceled2 === false) {
						ReadableStreamDefaultControllerEnqueue(branch2.controller, chunk2);
					}
					reading = false;
					if (readAgain === true) {
						pullAlgorithm();
					}
				});
			},
			closeSteps: () => {
				reading = false;
				if (canceled1 === false) {
					ReadableStreamDefaultControllerClose(branch1.controller);
				}
				if (canceled2 === false) {
					ReadableStreamDefaultControllerClose(branch2.controller);
				}
				if (canceled1 === false || canceled2 === false) {
					resolvePromise(cancelPromise, undefined);
				}
			},
			errorSteps: () => {
				reading = false;
			}
		};
		ReadableStreamDefaultReaderRead(reader, readRequest);
		return Promise.resolve(undefined);
	}
	function cancel1Algorithm(reason) {
		canceled1 = true;
		reason1 = reason;
		if (canceled2 === true) {
			const compositeReason = [reason1, reason2];
			const cancelResult = ReadableStreamCancel(stream, compositeReason);
			resolvePromise(cancelPromise, cancelResult);
		}
		return cancelPromise.promise;
	}
	function cancel2Algorithm(reason) {
		canceled2 = true;
		reason2 = reason;
		if (canceled1 === true) {
			const compositeReason = [reason1, reason2];
			const cancelResult = ReadableStreamCancel(stream, compositeReason);
			resolvePromise(cancelPromise, cancelResult);
		}
		return cancelPromise.promise;
	}
	function startAlgorithm() {
	}
	branch1 = CreateReadableStream(startAlgorithm, pullAlgorithm, cancel1Algorithm);
	branch2 = CreateReadableStream(startAlgorithm, pullAlgorithm, cancel2Algorithm);
	reader.closedPromise.promise.catch(r => {
		ReadableStreamDefaultControllerError(branch1.controller, r);
		ReadableStreamDefaultControllerError(branch2.controller, r);
		if ((canceled1 === false) || (canceled2 === false))
			resolvePromise(cancelPromise, undefined);
	});
	return [ branch1, branch2 ];
}
function ReadableByteStreamTee(stream, cloneForBranch2) {
	assert(ReadableStreamBrand.has(stream.self));
	let reader = AcquireReadableStreamDefaultReader(stream);
	let reading = false;
	let readAgainForBranch1 = false;
	let readAgainForBranch2 = false;
	let canceled1 = false;
	let canceled2 = false;
	let reason1;
	let reason2;
	let branch1;
	let branch2;
	let cancelPromise = createPromise();
	function forwardReaderError(thisReader) {
		thisReader.closedPromise.promise.catch(r => {
			if (thisReader !== reader) {
				return;
			}
			ReadableByteStreamControllerError(branch1.controller, r);
			ReadableByteStreamControllerError(branch2.controller, r);
			if (canceled1 === false || canceled2 === false) {
				resolvePromise(cancelPromise, undefined);
			}
		});
	}
	function pullWithDefaultReader() {
		if (ReadableStreamBYOBReaderBrand.has(reader.self)) {
			assert(reader.readIntoRequests.length === 0);
			ReadableStreamBYOBReaderRelease(reader);
			reader = AcquireReadableStreamDefaultReader(stream);
			forwardReaderError(reader);
		}
		const readRequest = {
			chunkSteps: chunk => {
				// This needs to be delayed a microtask because it takes at least a microtask to detect errors (using
				// reader.closedPromise below), and we want errors in stream to error both branches immediately. We cannot let
				// successful synchronously-available reads get ahead of asynchronously-available errors.
				queueMicrotask(() => {
					readAgainForBranch1 = false;
					readAgainForBranch2 = false;
					const chunk1 = chunk;
					let chunk2 = chunk;
					if (canceled1 === false && canceled2 === false) {
						try {
							chunk2 = CloneAsUint8Array(chunk);
						}
						catch (e) {
							ReadableByteStreamControllerError(branch1.controller, e);
							ReadableByteStreamControllerError(branch2.controller, e);
							resolvePromise(cancelPromise, ReadableStreamCancel(stream, e));
							return;
						}
					}
					if (canceled1 === false) {
						ReadableByteStreamControllerEnqueue(branch1.controller, chunk1);
					}
					if (canceled2 === false) {
						ReadableByteStreamControllerEnqueue(branch2.controller, chunk2);
					}
					reading = false;
					if (readAgainForBranch1 === true) {
						pull1Algorithm();
					} else if (readAgainForBranch2 === true) {
						pull2Algorithm();
					}
				});
			},
			closeSteps: () => {
				reading = false;
				if (canceled1 === false) {
					ReadableByteStreamControllerClose(branch1.controller);
				}
				if (canceled2 === false) {
					ReadableByteStreamControllerClose(branch2.controller);
				}
				if (branch1.controller.pendingPullIntos.length > 0) {
					ReadableByteStreamControllerRespond(branch1.controller, 0);
				}
				if (branch2.controller.pendingPullIntos.length > 0) {
					ReadableByteStreamControllerRespond(branch2.controller, 0);
				}
				if (canceled1 === false || canceled2 === false) {
					resolvePromise(cancelPromise, undefined);
				}
			},
			errorSteps: () => {
				reading = false;
			}
		};
		ReadableStreamDefaultReaderRead(reader, readRequest);
	}
	function pullWithBYOBReader(view, forBranch2) {
		if (ReadableStreamDefaultReaderBrand.has(reader.self)) {
			assert(reader.readRequests.length === 0);
			ReadableStreamDefaultReaderRelease(reader);
			reader = AcquireReadableStreamBYOBReader(stream);
			forwardReaderError(reader);
		}
		const byobBranch = forBranch2 ? branch2 : branch1;
		const otherBranch = forBranch2 ? branch1 : branch2;
		const readIntoRequest = {
			chunkSteps: chunk => {
				// This needs to be delayed a microtask because it takes at least a microtask to detect errors (using
				// reader.closedPromise below), and we want errors in stream to error both branches immediately. We cannot let
				// successful synchronously-available reads get ahead of asynchronously-available errors.
				queueMicrotask(() => {
					readAgainForBranch1 = false;
					readAgainForBranch2 = false;
					const byobCanceled = forBranch2 ? canceled2 : canceled1;
					const otherCanceled = forBranch2 ? canceled1 : canceled2;
					if (otherCanceled === false) {
						let clonedChunk;
						try {
							clonedChunk = CloneAsUint8Array(chunk);
						} 
						catch (e) {
							ReadableByteStreamControllerError(byobBranch.controller, e);
							ReadableByteStreamControllerError(otherBranch.controller, e);
							cancelPromise.resolvePromise(ReadableStreamCancel(stream, e));
							return;
						}
						if (byobCanceled === false) {
							ReadableByteStreamControllerRespondWithNewView(byobBranch.controller, chunk);
						}
						ReadableByteStreamControllerEnqueue(otherBranch.controller, clonedChunk);
					}
					else if (byobCanceled === false) {
						ReadableByteStreamControllerRespondWithNewView(byobBranch.controller, chunk);
					}
					reading = false;
					if (readAgainForBranch1 === true) {
						pull1Algorithm();
					}
					else if (readAgainForBranch2 === true) {
						pull2Algorithm();
					}
				});
			},
			closeSteps: chunk => {
				reading = false;
				const byobCanceled = forBranch2 ? canceled2 : canceled1;
				const otherCanceled = forBranch2 ? canceled1 : canceled2;
				if (byobCanceled === false) {
					ReadableByteStreamControllerClose(byobBranch.controller);
				}
				if (otherCanceled === false) {
					ReadableByteStreamControllerClose(otherBranch.controller);
				}
				if (chunk !== undefined) {
					assert(chunk.byteLength === 0);
					if (byobCanceled === false) {
						ReadableByteStreamControllerRespondWithNewView(byobBranch.controller, chunk);
					}
					if (otherCanceled === false && otherBranch.controller.pendingPullIntos.length > 0) {
						ReadableByteStreamControllerRespond(otherBranch.controller, 0);
					}
				}
				if (byobCanceled === false || otherCanceled === false) {
					resolvePromise(cancelPromise, undefined);
				}
			},
			errorSteps: () => {
				reading = false;
			}
		};
		ReadableStreamBYOBReaderRead(reader, view, 1, readIntoRequest);
	}
	function pull1Algorithm() {
		if (reading === true) {
			readAgainForBranch1 = true;
			return Promise.resolve(undefined);
		}
		reading = true;
		const byobRequest = ReadableByteStreamControllerGetBYOBRequest(branch1.controller);
		if (byobRequest === null) {
			pullWithDefaultReader();
		}
		else {
			pullWithBYOBReader(byobRequest.view, false);
		}
		return Promise.resolve(undefined);
	}
	function pull2Algorithm() {
		if (reading === true) {
			readAgainForBranch2 = true;
			return Promise.resolve(undefined);
		}
		reading = true;
		const byobRequest = ReadableByteStreamControllerGetBYOBRequest(branch2.controller);
		if (byobRequest === null) {
			pullWithDefaultReader();
		}
		else {
			pullWithBYOBReader(byobRequest.view, true);
		}
		return Promise.resolve(undefined);
	}
	function cancel1Algorithm(reason) {
		canceled1 = true;
		reason1 = reason;
		if (canceled2 === true) {
			const compositeReason = [reason1, reason2];
			const cancelResult = ReadableStreamCancel(stream, compositeReason);
			resolvePromise(cancelPromise, cancelResult);
		}
		return cancelPromise.promise;
	}
	function cancel2Algorithm(reason) {
		canceled2 = true;
		reason2 = reason;
		if (canceled1 === true) {
			const compositeReason = [reason1, reason2];
			const cancelResult = ReadableStreamCancel(stream, compositeReason);
			resolvePromise(cancelPromise, cancelResult);
		}
		return cancelPromise.promise;
	}
	function startAlgorithm() {
	}
	branch1 = CreateReadableByteStream(startAlgorithm, pull1Algorithm, cancel1Algorithm);
	branch2 = CreateReadableByteStream(startAlgorithm, pull2Algorithm, cancel2Algorithm);
	forwardReaderError(reader);
	return [branch1, branch2];
}

// 4.9.2 Interfacing with controllers 
function ReadableStreamAddReadIntoRequest(stream, readRequest) {
	const reader = stream.reader;
	assert(ReadableStreamBYOBReaderBrand.has(reader.self));
	const state = stream.state;	
	assert((state == "readable") || (state == "closed"));
	reader.readIntoRequests.push(readRequest);
}
function ReadableStreamAddReadRequest(stream, readRequest) {
	const reader = stream.reader;
	assert(ReadableStreamDefaultReaderBrand.has(reader.self));
	const state = stream.state;	
	assert(state == "readable");
	reader.readRequests.push(readRequest);
}
function ReadableStreamCancel(stream, reason) {
	stream.disturbed = true;
	const state = stream.state;	
	if (state == "closed")
		return Promise.resolve(undefined);
	if (state == "errored")
		return Promise.reject(stream.storedError);
	ReadableStreamClose(stream);
	const reader = stream.reader;
	if (reader && ReadableStreamBYOBReaderBrand.has(reader.self)) {
		let readIntoRequests = reader.readIntoRequests;
		reader.readIntoRequests = [];
		readIntoRequests.forEach(readIntoRequest => {
			readIntoRequest.closeSteps(undefined);
		});
	}
	let promise = stream.controller.cancelSteps(reason);
	let then = promise.then(value => {
		return undefined;
	});
	return then;
}
function ReadableStreamClose(stream) {
	const state = stream.state;	
	assert(state == "readable");
	stream.state = "closed";
	const reader = stream.reader;
	if (reader === undefined)
		return;
	resolvePromise(reader.closedPromise, undefined);
	if (ReadableStreamDefaultReaderBrand.has(reader.self)) {
		let readRequests = reader.readRequests;
		reader.readRequests = [];
		readRequests.forEach(readRequest => {
			readRequest.closeSteps();
		});
	}
}
function ReadableStreamError(stream, e) {
	const state = stream.state;	
	assert(state == "readable");
	stream.state = "errored";
	trace(`ReadableStreamError ${e}\n`);
	stream.storedError = e;
	const reader = stream.reader;
	if (reader === undefined)
		return;
	rejectPromise(reader.closedPromise, e);
	handlePromise(reader.closedPromise);
	if (ReadableStreamDefaultReaderBrand.has(reader.self))
		ReadableStreamDefaultReaderErrorReadRequests(reader, e);
	else
		ReadableStreamBYOBReaderErrorReadIntoRequests(reader, e);
}
function ReadableStreamFulfillReadIntoRequest(stream, chunk, done) {
	assert(ReadableStreamHasBYOBReader(stream));
	const reader = stream.reader;
	assert(reader.readIntoRequests.length > 0);
	let readIntoRequest = reader.readIntoRequests.shift();
	if (done)
		readIntoRequest.closeSteps(chunk);
	else
		readIntoRequest.chunkSteps(chunk);
}
function ReadableStreamFulfillReadRequest(stream, chunk, done) {
	assert(ReadableStreamHasDefaultReader(stream));
	const reader = stream.reader;
	assert(reader.readRequests.length > 0);
	let readRequest = reader.readRequests.shift();
	if (done)
		readRequest.closeSteps();
	else
		readRequest.chunkSteps(chunk);
}
function ReadableStreamGetNumReadIntoRequests(stream) {
	assert(ReadableStreamHasBYOBReader(stream));
	return stream.reader.readIntoRequests.length;
}
function ReadableStreamGetNumReadRequests(stream) {
	assert(ReadableStreamHasDefaultReader(stream));
	return stream.reader.readRequests.length;
}
function ReadableStreamHasBYOBReader(stream) {
	const reader = stream.reader;
	if (reader)
		return ReadableStreamBYOBReaderBrand.has(reader.self);
	return false;
}
function ReadableStreamHasDefaultReader(stream) {
	const reader = stream.reader;
	if (reader)
		return ReadableStreamDefaultReaderBrand.has(reader.self);
	return false;
}

// 4.9.3 Readers
function ReadableStreamReaderGenericCancel(reader, reason) {
	const stream = reader.stream;
	assert(stream !== undefined);
	return ReadableStreamCancel(stream, reason);
}	
function ReadableStreamReaderGenericInitialize(reader, stream) {
	reader.stream = stream;
	stream.reader = reader;
	const state = stream.state;
	if (state == "readable") {
		reader.closedPromise = createPromise();
	}
	else if (state == "closed") {
		reader.closedPromise = createResolvedPromise(undefined);
	}
	else {
		assert(state == "errored");
		reader.closedPromise = createRejectedPromise(stream.storedError);
		handlePromise(reader.closedPromise);
	}
}
function ReadableStreamReaderGenericRelease(reader) {
	const stream = reader.stream;
	assert(stream !== undefined);
	assert(stream.reader === reader);
	const state = stream.state;
	if (state == "readable")
		rejectPromise(reader.closedPromise, new TypeError("release"));
	else 
		reader.closedPromise = createRejectedPromise(new TypeError("release"));
	handlePromise(reader.closedPromise);
	stream.controller.releaseSteps();
	stream.reader = undefined;
	reader.stream = undefined;
}
function ReadableStreamBYOBReaderErrorReadIntoRequests(reader, e) {
	let readIntoRequests = reader.readIntoRequests;
	reader.readIntoRequests = [];
	readIntoRequests.forEach(readIntoRequest => {
		readIntoRequest.errorSteps(e);
	});
}
function ReadableStreamBYOBReaderRead(reader, view, min, readIntoRequest) {
	const stream = reader.stream;
	assert(stream !== undefined);
	stream.disturbed = true;
	if (stream.state == "errored") 
		readIntoRequest.errorSteps(stream.storedError);
	else
		ReadableByteStreamControllerPullInto(stream.controller, view, min, readIntoRequest);
}
function ReadableStreamBYOBReaderRelease(reader) {
	ReadableStreamReaderGenericRelease(reader);
	ReadableStreamBYOBReaderErrorReadIntoRequests(reader, new TypeError("release"));
}
function ReadableStreamDefaultReaderErrorReadRequests(reader, e) {
	let readRequests = reader.readRequests;
	reader.readRequests = [];
	readRequests.forEach(readRequest => {
		readRequest.errorSteps(e);
	});
}
function ReadableStreamDefaultReaderRead(reader, readRequest) {
	const stream = reader.stream;
	assert(stream !== undefined);
	stream.disturbed = true;
	const state = stream.state;
	if (state == "closed")
		readRequest.closeSteps();
	else if (state == "errored")
		readRequest.errorSteps(stream.storedError);
	else {
		assert(state == "readable");
		stream.controller.pullSteps(readRequest);
	}
}
function ReadableStreamDefaultReaderRelease(reader) {
	ReadableStreamReaderGenericRelease(reader);
	ReadableStreamDefaultReaderErrorReadRequests(reader, new TypeError("release"));
}
function SetUpReadableStreamBYOBReader(reader, stream) {
	if (IsReadableStreamLocked(stream))
		throw new TypeError("stream locked");
	if (!ReadableByteStreamControllerBrand.has(stream.controller.self))
		throw new TypeError("invalid controller");
	ReadableStreamReaderGenericInitialize(reader, stream);
	reader.readIntoRequests =	[];
}
function SetUpReadableStreamDefaultReader(reader, stream) {
	if (IsReadableStreamLocked(stream))
		throw new TypeError("stream locked");
	ReadableStreamReaderGenericInitialize(reader, stream);
	reader.readRequests =	[];
}

// 4.9.4 Default controllers
function ReadableStreamDefaultControllerCallPullIfNeeded(controller) {
	if (!ReadableStreamDefaultControllerShouldCallPull(controller))
		return;
	if (controller.pulling) {
		controller.pullAgain = true;
		return;
	}
	assert(controller.pullAgain === false);
	controller.pulling = true;
	let pullPromise = controller.pullAlgorithm(controller.self);
	pullPromise.then(
		value => {
			controller.pulling = false;
			if (controller.pullAgain) {
				controller.pullAgain = false;
				ReadableStreamDefaultControllerCallPullIfNeeded(controller);
			}
		},
		reason => {
			ReadableStreamDefaultControllerError(controller, reason);
		}
	);
}
function ReadableStreamDefaultControllerShouldCallPull(controller){
	const stream = controller.stream;
	if (!ReadableStreamDefaultControllerCanCloseOrEnqueue(controller))
		return false;
	if (!controller.started)
		return false;
	if (IsReadableStreamLocked(stream) && (ReadableStreamGetNumReadRequests(stream) > 0))
		return true;
	const desiredSize = ReadableStreamDefaultControllerGetDesiredSize(controller);
	assert(desiredSize !== null);
	return desiredSize > 0;
}
function ReadableStreamDefaultControllerClearAlgorithms(controller) {
	controller.pullAlgorithm = undefined;
	controller.cancelAlgorithm = undefined;
	controller.strategySizeAlgorithm = undefined;
}
function ReadableStreamDefaultControllerClose(controller) {
	if (!ReadableStreamDefaultControllerCanCloseOrEnqueue(controller))
		return;
	const stream = controller.stream;
	controller.closeRequested = true;
	if (controller.queue.length == 0) {
		ReadableStreamDefaultControllerClearAlgorithms(controller);
		ReadableStreamClose(stream);
	}
}
function ReadableStreamDefaultControllerEnqueue(controller, chunk) {
	if (!ReadableStreamDefaultControllerCanCloseOrEnqueue(controller))
		return;
	const stream = controller.stream;
	if (IsReadableStreamLocked(stream) && (ReadableStreamGetNumReadRequests(stream) > 0))
		ReadableStreamFulfillReadRequest(stream, chunk, false);
	else {
		try {
			let chunkSize = controller.strategySizeAlgorithm(chunk);
			let enqueueResult = EnqueueValueWithSize(controller, chunk, chunkSize);
		}
		catch(e) {
			ReadableStreamDefaultControllerError(controller, e);
			throw e;
		}
	}
	ReadableStreamDefaultControllerCallPullIfNeeded(controller);
}
function ReadableStreamDefaultControllerError(controller, e) {
	const stream = controller.stream;
	if (stream.state != "readable")
		return;
	ResetQueue(controller);
	ReadableStreamDefaultControllerClearAlgorithms(controller);
	ReadableStreamError(stream, e);
}
function ReadableStreamDefaultControllerGetDesiredSize(controller) {
	const state = controller.stream.state;
	if (state == "errored")
		return null;
	if (state == "closed")
		return 0;
	return controller.strategyHWM - controller.queueTotalSize;
}
function ReadableStreamDefaultControllerHasBackpressure(controller)	{
	return !ReadableStreamDefaultControllerShouldCallPull(controller);
}
function ReadableStreamDefaultControllerCanCloseOrEnqueue(controller) {
	const state = controller.stream.state;
	return (!controller.closeRequested && (state == "readable"));
}
function SetUpReadableStreamDefaultController(stream, controller, startAlgorithm, pullAlgorithm, cancelAlgorithm, highWaterMark, sizeAlgorithm) {
	assert(stream.controller === undefined);
	controller.stream = stream;
	
 	controller.queue = controller.queueTotalSize = undefined;
	ResetQueue(controller);
	controller.started = false;
	controller.closeRequested = false;
	controller.pullAgain = false;
	controller.pulling = false;
	controller.strategyHWM = highWaterMark;
	controller.strategySizeAlgorithm = sizeAlgorithm;
	controller.pullAlgorithm = pullAlgorithm;
	controller.cancelAlgorithm = cancelAlgorithm;
	stream.controller = controller;
	
	const startResult = startAlgorithm(controller.self);
	const startPromise = Promise.resolve(startResult);
	startPromise.then(
		value => {
			controller.started = true;
			assert(controller.pulling === false);
			assert(controller.pullAgain === false);
			ReadableStreamDefaultControllerCallPullIfNeeded(controller);
		},
		reason => {
			ReadableStreamDefaultControllerError(controller, reason);
		}
	);
}
function SetUpReadableStreamDefaultControllerFromUnderlyingSource(stream, underlyingSource, underlyingSourceDict, highWaterMark, sizeAlgorithm) {
	const controller = new ReadableStreamDefaultController(ReadableStreamDefaultControllerBrand.token);
	const startAlgorithm = createCallback(underlyingSourceDict.start, underlyingSource);
	const pullAlgorithm = createPromiseCallback(underlyingSourceDict.pull, underlyingSource);
	const cancelAlgorithm = createPromiseCallback(underlyingSourceDict.cancel, underlyingSource);
	SetUpReadableStreamDefaultController(stream, controller, startAlgorithm, pullAlgorithm, cancelAlgorithm, highWaterMark, sizeAlgorithm);
}

// 4.9.4 Byte stream controllers
function ReadableByteStreamControllerCallPullIfNeeded(controller) {
	if (!ReadableByteStreamControllerShouldCallPull(controller))
		return;
	if (controller.pulling) {
		controller.pullAgain = true;
		return;
	}
	assert(controller.pullAgain === false);
	controller.pulling = true;
	let pullPromise = controller.pullAlgorithm(controller.self);
	pullPromise.then(
		value => {
			controller.pulling = false;
			if (controller.pullAgain) {
				controller.pullAgain = false;
				ReadableByteStreamControllerCallPullIfNeeded(controller);
			}
		},
		reason => {
			ReadableByteStreamControllerError(controller, reason);
		}
	);
}
function ReadableByteStreamControllerClearAlgorithms(controller) {
	controller.pullAlgorithm = undefined;
	controller.cancelAlgorithm = undefined;
}
function ReadableByteStreamControllerClearPendingPullIntos(controller) {
	ReadableByteStreamControllerInvalidateBYOBRequest(controller);
	controller.pendingPullIntos = [];
}
function ReadableByteStreamControllerClose(controller) {
	const stream = controller.stream;
	if (controller.closeRequested || (stream.state != "readable"))
		return;
	if (controller.queueTotalSize > 0) {
		controller.closeRequested = true;
		return;
	}
	if (controller.pendingPullIntos.length > 0) {
		const firstPendingPullInto = controller.pendingPullIntos[0];
    	if (firstPendingPullInto.bytesFilled % firstPendingPullInto.elementSize !== 0) {
			const e = new TypeError("Insufficient bytes to fill elements in the given buffer");
			ReadableByteStreamControllerError(controller, e);
			throw e;
		}
	}
	ReadableByteStreamControllerClearAlgorithms(controller);
	ReadableStreamClose(stream);
}	
function ReadableByteStreamControllerCommitPullIntoDescriptor(stream, pullIntoDescriptor) {
	const state = stream.state;
	assert(state != "errored");
	assert(pullIntoDescriptor.readerType != "none");
	let done = false;
	if (state == "closed") {
    	assert(pullIntoDescriptor.bytesFilled % pullIntoDescriptor.elementSize === 0);
		done = true;
	}
	const filledView = ReadableByteStreamControllerConvertPullIntoDescriptor(pullIntoDescriptor);
	if (pullIntoDescriptor.readerType == "default")
		ReadableStreamFulfillReadRequest(stream, filledView, done);
	else {
		assert(pullIntoDescriptor.readerType === "byob");
		ReadableStreamFulfillReadIntoRequest(stream, filledView, done);
	}
}
function ReadableByteStreamControllerConvertPullIntoDescriptor(pullIntoDescriptor) {
	const bytesFilled = pullIntoDescriptor.bytesFilled;
	const elementSize = pullIntoDescriptor.elementSize;
	assert(bytesFilled <= pullIntoDescriptor.byteLength);
	assert(bytesFilled % elementSize === 0);
	const buffer = TransferArrayBuffer(pullIntoDescriptor.buffer);
	return new pullIntoDescriptor.ViewConstructor(buffer, pullIntoDescriptor.byteOffset, bytesFilled / elementSize);
}
function ReadableByteStreamControllerEnqueue(controller, chunk) {
	const stream = controller.stream;
	if (controller.closeRequested || (stream.state != "readable"))
		return;
	const buffer = chunk.buffer;
	const byteOffset = chunk.byteOffset;
	const byteLength = chunk.byteLength;
	const transferredBuffer = TransferArrayBuffer(buffer);
	if (controller.pendingPullIntos.length > 0) {
		const firstPendingPullInto = controller.pendingPullIntos[0];
		if (IsDetachedBuffer(firstPendingPullInto.buffer)) throw new TypeError("detached buffer");
		ReadableByteStreamControllerInvalidateBYOBRequest(controller);
		firstPendingPullInto.buffer = TransferArrayBuffer(firstPendingPullInto.buffer);
		if (firstPendingPullInto.readerType == "none")
			ReadableByteStreamControllerEnqueueDetachedPullIntoToQueue(controller, firstPendingPullInto);
	}
	if (ReadableStreamHasDefaultReader(stream)) {
		ReadableByteStreamControllerProcessReadRequestsUsingQueue(controller);
		if (ReadableStreamGetNumReadRequests(stream) === 0) {
			assert(controller.pendingPullIntos.length === 0);
			ReadableByteStreamControllerEnqueueChunkToQueue(controller, transferredBuffer, byteOffset, byteLength);
		}
		else {
			assert(controller.queue.length === 0);
			if (controller.pendingPullIntos.length > 0) {
				assert(controller.pendingPullIntos[0].readerType == "default");
				ReadableByteStreamControllerShiftPendingPullInto(controller);
			}
			const transferredView = new Uint8Array(transferredBuffer, byteOffset, byteLength);
			ReadableStreamFulfillReadRequest(stream, transferredView, false);
		}
	}
	else if (ReadableStreamHasBYOBReader(stream)) {
		ReadableByteStreamControllerEnqueueChunkToQueue(controller, transferredBuffer, byteOffset, byteLength);
		ReadableByteStreamControllerProcessPullIntoDescriptorsUsingQueue(controller)
	}
	else {
		assert(IsReadableStreamLocked(stream) === false);
		ReadableByteStreamControllerEnqueueChunkToQueue(controller, transferredBuffer, byteOffset, byteLength);
	}
	ReadableByteStreamControllerCallPullIfNeeded(controller);
}
function ReadableByteStreamControllerEnqueueChunkToQueue(controller, buffer, byteOffset, byteLength) {
	controller.queue.push({ buffer, byteOffset, byteLength });
	controller.queueTotalSize += byteLength;
}
function ReadableByteStreamControllerEnqueueClonedChunkToQueue(controller, buffer, byteOffset, byteLength) {
	let clonedChunk;
	try {
		clonedChunk = buffer.slice(byteOffset, byteOffset + byteLength);
	}
	catch(e) {
		ReadableByteStreamControllerError(controller, e);
		throw e;
	}
	ReadableByteStreamControllerEnqueueChunkToQueue(controller, clonedChunk, 0, byteLength)
}
function ReadableByteStreamControllerEnqueueDetachedPullIntoToQueue(controller, pullIntoDescriptor) {
	assert(pullIntoDescriptor.readerType == "none");
	if (pullIntoDescriptor.bytesFilled > 0)
		ReadableByteStreamControllerEnqueueClonedChunkToQueue(controller, pullIntoDescriptor.buffer, pullIntoDescriptor.byteOffset, pullIntoDescriptor.bytesFilled);
	ReadableByteStreamControllerShiftPendingPullInto(controller);
}
function ReadableByteStreamControllerError(controller, e) {
	const stream = controller.stream;
	if (stream.state != "readable")
		return;
	ReadableByteStreamControllerClearPendingPullIntos(controller);
	ResetQueue(controller);
	ReadableByteStreamControllerClearAlgorithms(controller);
	ReadableStreamError(stream, e);
}
function ReadableByteStreamControllerFillHeadPullIntoDescriptor(controller, size, pullIntoDescriptor) {
	assert((controller.pendingPullIntos.length == 0) || (controller.pendingPullIntos[0] === pullIntoDescriptor));
	assert(controller.byobRequest === null);
	pullIntoDescriptor.bytesFilled += size;
}
function ReadableByteStreamControllerFillPullIntoDescriptorFromQueue(controller, pullIntoDescriptor) {
	const maxBytesToCopy = Math.min(controller.queueTotalSize, pullIntoDescriptor.byteLength - pullIntoDescriptor.bytesFilled);
	const maxBytesFilled = pullIntoDescriptor.bytesFilled + maxBytesToCopy;
	let totalBytesToCopyRemaining = maxBytesToCopy;
	let ready = false;
	assert(pullIntoDescriptor.bytesFilled < pullIntoDescriptor.minimumFill);
	const remainderBytes = maxBytesFilled % pullIntoDescriptor.elementSize;
	const maxAlignedBytes = maxBytesFilled - remainderBytes;
	if (maxAlignedBytes >= pullIntoDescriptor.minimumFill) {
		totalBytesToCopyRemaining = maxAlignedBytes - pullIntoDescriptor.bytesFilled;
		ready = true;
	}
	const queue = controller.queue;
	while (totalBytesToCopyRemaining > 0) {
		const headOfQueue = queue[0];
		const bytesToCopy = Math.min(totalBytesToCopyRemaining, headOfQueue.byteLength);
		const destStart = pullIntoDescriptor.byteOffset + pullIntoDescriptor.bytesFilled;
		CopyDataBlockBytes(pullIntoDescriptor.buffer, destStart, headOfQueue.buffer, headOfQueue.byteOffset, bytesToCopy);
		if (headOfQueue.byteLength === bytesToCopy)
			queue.shift();
		else {
			headOfQueue.byteOffset += bytesToCopy;
			headOfQueue.byteLength -= bytesToCopy;
		}
		controller.queueTotalSize -= bytesToCopy;
		ReadableByteStreamControllerFillHeadPullIntoDescriptor(controller, bytesToCopy, pullIntoDescriptor);
		totalBytesToCopyRemaining -= bytesToCopy;
	}
	if (!ready) {
		assert(controller.queueTotalSize == 0);
		assert(pullIntoDescriptor.bytesFilled > 0);
		assert(pullIntoDescriptor.bytesFilled < pullIntoDescriptor.minimumFill);
	}
	return ready
}
function ReadableByteStreamControllerFillReadRequestFromQueue(controller, readRequest) {
	assert(controller.queueTotalSize > 0);
	const entry = controller.queue.shift();
	controller.queueTotalSize -= entry.byteLength;
	ReadableByteStreamControllerHandleQueueDrain(controller);
	const view = new Uint8Array(entry.buffer, entry.byteOffset, entry.byteLength);
	readRequest.chunkSteps(view);
}
function ReadableByteStreamControllerGetBYOBRequest(controller) {
	if ((controller.byobRequest === null) && (controller.pendingPullIntos.length > 0)) {
		const firstDescriptor = controller.pendingPullIntos[0];
		const view = new Uint8Array(firstDescriptor.buffer, firstDescriptor.byteOffset + firstDescriptor.bytesFilled, firstDescriptor.byteLength - firstDescriptor.bytesFilled);
		const byobRequest = new ReadableStreamBYOBRequest(ReadableStreamBYOBRequestBrand.token);
		byobRequest.controller = controller;
		byobRequest.view = view;
		controller.byobRequest = byobRequest;
	}
	return controller.byobRequest;
}
function ReadableByteStreamControllerGetDesiredSize(controller) {
	const state = controller.stream.state;
	if (state == "errored") return null;
	if (state == "closed") return 0;
	return controller.strategyHWM - controller.queueTotalSize;
}
function ReadableByteStreamControllerHandleQueueDrain(controller) {
	assert(controller.stream.state == "readable");
	if ((controller.queueTotalSize == 0) && (controller.closeRequested)) {
		ReadableByteStreamControllerClearAlgorithms(controller);
		ReadableStreamClose(controller.stream);
	}
	else {
		ReadableByteStreamControllerCallPullIfNeeded(controller);
	}
}
function ReadableByteStreamControllerInvalidateBYOBRequest(controller) {
	if (controller.byobRequest === null) return;
	controller.byobRequest.controller = undefined;
	controller.byobRequest.view = null;
	controller.byobRequest = null;
}
function ReadableByteStreamControllerProcessPullIntoDescriptorsUsingQueue(controller) {
	assert(controller.closeRequested === false);
	while (controller.pendingPullIntos.length > 0) {
		if (controller.queueTotalSize == 0) return;
		const pullIntoDescriptor = controller.pendingPullIntos[0];
		if (ReadableByteStreamControllerFillPullIntoDescriptorFromQueue(controller, pullIntoDescriptor)) {
			ReadableByteStreamControllerShiftPendingPullInto(controller);
			ReadableByteStreamControllerCommitPullIntoDescriptor(controller.stream, pullIntoDescriptor)
		}
	}
}
function ReadableByteStreamControllerProcessReadRequestsUsingQueue(controller) {
	const reader = controller.stream.reader;
	assert(ReadableStreamDefaultReaderBrand.has(reader.self));
	while (reader.readRequests.length > 0) {
		if (controller.queueTotalSize == 0) return;
		const readRequest = reader.readRequests.shift();
		ReadableByteStreamControllerFillReadRequestFromQueue(controller, readRequest);
	}
}
function ReadableByteStreamControllerPullInto(controller, view, min, readIntoRequest) {
	const stream = controller.stream;
	const ViewConstructor = view.constructor;
	const elementSize = (ViewConstructor !== DataView) ? ViewConstructor.BYTES_PER_ELEMENT : 1;
	const minimumFill = min * elementSize;
	assert(minimumFill >= elementSize && minimumFill <= view.byteLength);
	assert(minimumFill % elementSize === 0);
	const byteOffset = view.byteOffset;
	const byteLength = view.byteLength;
	let buffer;
	try {
		buffer = TransferArrayBuffer(view.buffer);
	}
	catch (e) {
		readIntoRequest.errorSteps(e);
		return;
	}
	const pullIntoDescriptor = {
		buffer,
		bufferByteLength: buffer.byteLength,
		byteOffset,
		byteLength,
		bytesFilled: 0,
    	minimumFill,
		elementSize,
		ViewConstructor,
		readerType: 'byob'
	};
	if (controller.pendingPullIntos.length > 0) {
		controller.pendingPullIntos.push(pullIntoDescriptor);
		ReadableStreamAddReadIntoRequest(stream, readIntoRequest);
		return;
	}
	if (stream.state == "closed") {
		const emptyView = new ViewConstructor(pullIntoDescriptor.buffer, pullIntoDescriptor.byteOffset, 0);
		readIntoRequest.closeSteps(emptyView);
		return;
	}
	if (controller.queueTotalSize > 0) {
		if (ReadableByteStreamControllerFillPullIntoDescriptorFromQueue(controller, pullIntoDescriptor)) {
			const filledView = ReadableByteStreamControllerConvertPullIntoDescriptor(pullIntoDescriptor);
			ReadableByteStreamControllerHandleQueueDrain(controller);
			readIntoRequest.chunkSteps(filledView);
			return;
		}
		if (controller.closeRequested) {
			const e = new TypeError("closed");
			ReadableByteStreamControllerError(controller, e);
			readIntoRequest.errorSteps(e);
			return;
		}
	}
	controller.pendingPullIntos.push(pullIntoDescriptor);
	ReadableStreamAddReadIntoRequest(stream, readIntoRequest);
	ReadableByteStreamControllerCallPullIfNeeded(controller);
}
function ReadableByteStreamControllerRespond(controller, bytesWritten) {
	assert(controller.pendingPullIntos.length > 0);
	const firstDescriptor = controller.pendingPullIntos[0];
	const state = controller.stream.state;
	if (state == "closed") {
		if (bytesWritten != 0) throw new TypeError("closed");
	}
	else {
		assert(state == "readable");
		if (bytesWritten == 0) throw new TypeError("zero");
		if (firstDescriptor.bytesFilled + bytesWritten > firstDescriptor.byteLength)
			throw new RangeError("too long");
	}
	firstDescriptor.buffer = TransferArrayBuffer(firstDescriptor.buffer);
	ReadableByteStreamControllerRespondInternal(controller, bytesWritten);
}
function ReadableByteStreamControllerRespondInClosedState(controller, firstDescriptor) {
	assert(firstDescriptor.bytesFilled % firstDescriptor.elementSize === 0);
	if (firstDescriptor.readerType == "none")
		ReadableByteStreamControllerShiftPendingPullInto(controller);
	const stream = controller.stream;
	if (ReadableStreamHasBYOBReader(stream)) {
		while (ReadableStreamGetNumReadIntoRequests(stream) > 0) {
			const pullIntoDescriptor = ReadableByteStreamControllerShiftPendingPullInto(controller);
			ReadableByteStreamControllerCommitPullIntoDescriptor(stream, pullIntoDescriptor);
		}
	}
}
function ReadableByteStreamControllerRespondInReadableState(controller, bytesWritten, pullIntoDescriptor) {
	assert(pullIntoDescriptor.bytesFilled + bytesWritten <= pullIntoDescriptor.byteLength);
	ReadableByteStreamControllerFillHeadPullIntoDescriptor(controller, bytesWritten, pullIntoDescriptor);
	if (pullIntoDescriptor.readerType == "none") {
		ReadableByteStreamControllerEnqueueDetachedPullIntoToQueue(controller, pullIntoDescriptor);
		ReadableByteStreamControllerProcessPullIntoDescriptorsUsingQueue(controller);
		return;
	}
	if (pullIntoDescriptor.bytesFilled < pullIntoDescriptor.minimumFill) return;
	ReadableByteStreamControllerShiftPendingPullInto(controller);
	const remainderSize = pullIntoDescriptor.bytesFilled % pullIntoDescriptor.elementSize;
	if (remainderSize > 0) {
		const end = pullIntoDescriptor.byteOffset + pullIntoDescriptor.bytesFilled;
		ReadableByteStreamControllerEnqueueClonedChunkToQueue(controller, pullIntoDescriptor.buffer, end - remainderSize, remainderSize);
	}
	pullIntoDescriptor.bytesFilled -= remainderSize;
	ReadableByteStreamControllerCommitPullIntoDescriptor(controller.stream, pullIntoDescriptor);
	ReadableByteStreamControllerProcessPullIntoDescriptorsUsingQueue(controller);
}
function ReadableByteStreamControllerRespondInternal(controller, bytesWritten) {
	const firstDescriptor = controller.pendingPullIntos[0];
	assert(CanTransferArrayBuffer(firstDescriptor.buffer));
	ReadableByteStreamControllerInvalidateBYOBRequest(controller);
	const state = controller.stream.state;
	if (state == "closed") {
		assert(bytesWritten == 0);
		ReadableByteStreamControllerRespondInClosedState(controller, firstDescriptor);
	}
	else {
		assert(state == "readable");
		assert(bytesWritten > 0);
		ReadableByteStreamControllerRespondInReadableState(controller, bytesWritten, firstDescriptor);
	}
	ReadableByteStreamControllerCallPullIfNeeded(controller);
}
function ReadableByteStreamControllerRespondWithNewView(controller, view) {
	assert(controller.pendingPullIntos.length > 0);
	assert(!IsDetachedBuffer(view.buffer));
	const firstDescriptor = controller.pendingPullIntos[0];
	const state = controller.stream.state;
	if (state == "closed") {
		if (view.byteLength != 0) throw new TypeError("closed");
	}
	else {
		assert(state == "readable");
		if (view.byteLength == 0) throw new TypeError("zero");
	}
	if (firstDescriptor.byteOffset + firstDescriptor.bytesFilled != view.byteOffset)
		throw new RangeError("offset");
	if (firstDescriptor.bufferByteLength != view.buffer.byteLength)
		throw new RangeError("bufferByteLength");
	if (firstDescriptor.bytesFilled + view.byteLength > firstDescriptor.byteLength)
		throw new RangeError("byteLength");
	const viewByteLength = view.byteLength;
	firstDescriptor.buffer = TransferArrayBuffer(view.buffer);
	ReadableByteStreamControllerRespondInternal(controller, viewByteLength);
}
function ReadableByteStreamControllerShiftPendingPullInto(controller) {
	assert(controller.byobRequest == null);
	return controller.pendingPullIntos.shift();
}
function ReadableByteStreamControllerShouldCallPull(controller) {
	const stream = controller.stream;
	if (stream.state != "readable") return false;
	if (controller.closeRequested) return false;
	if (!controller.started) return false;
	if (ReadableStreamHasDefaultReader(stream) && (ReadableStreamGetNumReadRequests(stream) > 0)) return true;
	if (ReadableStreamHasBYOBReader(stream) && (ReadableStreamGetNumReadIntoRequests(stream) > 0)) return true;
	const desiredSize = ReadableByteStreamControllerGetDesiredSize(controller);
	assert(desiredSize != null);
	return desiredSize > 0;
}
function SetUpReadableByteStreamController(stream, controller, startAlgorithm, pullAlgorithm, cancelAlgorithm, highWaterMark, autoAllocateChunkSize) {
	assert(stream.controller === undefined);
	if (autoAllocateChunkSize !== undefined) {
		assert(Number.isInteger(autoAllocateChunkSize));
		assert(autoAllocateChunkSize > 0);
	}
	controller.stream = stream;
	
	controller.pullAgain = false;
	controller.pulling = false;
	controller.byobRequest = null;
	controller.queue = controller.queueTotalSize = undefined;
	ResetQueue(controller);
	controller.closeRequested = false;
	controller.started = false;
	controller.strategyHWM = highWaterMark;
	controller.pullAlgorithm = pullAlgorithm;
	controller.cancelAlgorithm = cancelAlgorithm;
	controller.autoAllocateChunkSize = autoAllocateChunkSize;
	controller.pendingPullIntos = [];
	stream.controller = controller;
	
	const startResult = startAlgorithm(controller.self);
	const startPromise = Promise.resolve(startResult);
	startPromise.then(
		value => {
			controller.started = true;
			assert(controller.pulling === false);
			assert(controller.pullAgain === false);
			ReadableByteStreamControllerCallPullIfNeeded(controller);
		},
		reason => {
			ReadableByteStreamControllerError(controller, reason);
		}
	);
}
function SetUpReadableByteStreamControllerFromUnderlyingSource(stream, underlyingSource, underlyingSourceDict, highWaterMark) {
	const controller = new ReadableByteStreamController(ReadableByteStreamControllerBrand.token);
	const startAlgorithm = createCallback(underlyingSourceDict.start, underlyingSource);
	const pullAlgorithm = createPromiseCallback(underlyingSourceDict.pull, underlyingSource);
	const cancelAlgorithm = createPromiseCallback(underlyingSourceDict.cancel, underlyingSource);
	const autoAllocateChunkSize = underlyingSourceDict.autoAllocateChunkSize;
	if (autoAllocateChunkSize === 0)
		throw new TypeError("zero");
	SetUpReadableByteStreamController(stream, controller, startAlgorithm, pullAlgorithm, cancelAlgorithm, highWaterMark, autoAllocateChunkSize);
}
function ReadableStreamFromIterable(iterable) {
	let stream;
	let iterator;
	
	let iteratorMethod = iterable[Symbol.asyncIterator];
	if ((iteratorMethod === undefined) || (iteratorMethod === null)) {
		iteratorMethod = iterable[Symbol.iterator];
		if (typeof iteratorMethod !== 'function')
			throw new TypeError("iterable[Symbol.iterator] is no function");
		const syncIterator = iteratorMethod.call(iterable);
		const syncIterable = {
    		[Symbol.iterator]: () => syncIterator
		};
	  	iterator = (async function* () {
    		return yield* syncIterable;
  		}());
	}
	else {
		if (typeof iteratorMethod !== 'function')
			throw new TypeError("iterable[Symbol.asyncIterator] is no function");
		iterator = iteratorMethod.call(iterable);
		if (typeof iterator !== 'object')
			throw new TypeError('iterator is no object');
	}
		
	const nextMethod = iterator.next;
	if (typeof nextMethod !== 'function')
		throw new TypeError('iterator.next is no object');

	const startAlgorithm = () => undefined;

	function pullAlgorithm() {
		let nextResult;
		try {
			nextResult = nextMethod.call(iterator);
		} catch (e) {
			return Promise.reject(e);
		}
		const nextPromise = Promise.resolve(nextResult);
		return nextPromise.then(iterResult => {
			if (typeof(iterResult) !== "object")
				throw new TypeError('The promise returned by the iterator.next() method must fulfill with an object');
			const done = Boolean(iterResult.done);
			if (done === true) {
				ReadableStreamDefaultControllerClose(stream.controller);
			} else {
				const value = iterResult.value;
				ReadableStreamDefaultControllerEnqueue(stream.controller, value);
			}
		});
	}

	function cancelAlgorithm(reason) {
		let returnMethod = iterator.return;
		if ((returnMethod === undefined) || (returnMethod === null))
			return Promise.resolve(undefined);
		if (typeof returnMethod !== 'function')
			return Promise.reject("iterator.return is no function");
		let returnResult;
		try {
			returnResult = returnMethod.call(iterator, reason);
		} catch (e) {
			return Promise.reject(e);
		}
		const returnPromise = Promise.resolve(returnResult);
		return returnPromise.then(result => {
			if (typeof(result) !== "object")
				throw new TypeError('The promise returned by the iterator.return() method must fulfill with an object');
			return undefined;
		});
	}

	stream = CreateReadableStream(startAlgorithm, pullAlgorithm, cancelAlgorithm, 0);
	return stream;
}

// 5.2 WritableStream
const WritableStreamBrand = new Brand("WritableStream");
class WritableStream {
	constructor(underlyingSink, strategy) {
		const stream = { 
			self:this
		};
		WritableStreamBrand.set(this, stream);
		if (underlyingSink == WritableStreamBrand.token)
			return stream;
		const underlyingSinkDict = underlyingSink ?? {};
		InitializeWritableStream(stream);
		let sizeAlgorithm = ExtractSizeAlgorithm(strategy);
		let highWaterMark = ExtractHighWaterMark(strategy, 1);
		SetUpWritableStreamDefaultControllerFromUnderlyingSink(stream, underlyingSink, underlyingSinkDict, highWaterMark, sizeAlgorithm);
	}	
	get locked() {
		const stream = WritableStreamBrand.get(this);
		return IsWritableStreamLocked(stream);
	}
	abort(reason) {
		const stream = WritableStreamBrand.get(this);
		if (!stream)
			return Promise.reject(new TypeError("stream locked"));
		if (IsWritableStreamLocked(stream))
			return Promise.reject(new TypeError("stream locked"));
		return WritableStreamAbort(stream, reason);
	}
	close() {
		const stream = WritableStreamBrand.get(this);
		if (IsWritableStreamLocked(stream))
			return Promise.reject(new TypeError("stream locked"));
		if (WritableStreamCloseQueuedOrInFlight(stream))
			return Promise.reject(new TypeError("stream closed"));
		return WritableStreamClose(stream);
	}
	getWriter() {
		return new WritableStreamDefaultWriter(this);
	}
}

// 5.3 WritableStreamDefaultWriter
const WritableStreamDefaultWriterBrand = new Brand("WritableStreamDefaultWriter");
class WritableStreamDefaultWriter {
	constructor(stream, token) {
		const writer = { self:this };
		WritableStreamDefaultWriterBrand.set(this, writer);
		stream = WritableStreamBrand.get(stream, "stream");
		SetUpWritableStreamDefaultWriter(writer, stream);
		if (token === WritableStreamDefaultWriterBrand.token)
			return writer;
	}
	get closed() {
		const writer = WritableStreamDefaultWriterBrand.get(this);
		return writer.closedPromise.promise;
	}
	get desiredSize() {
		const writer = WritableStreamDefaultWriterBrand.get(this);
		const stream = writer.stream;
		if (stream === undefined)
			throw new TypeError("no stream");
		return WritableStreamDefaultWriterGetDesiredSize(writer);
	}
	get ready() {
		const writer = WritableStreamDefaultWriterBrand.get(this);
		return writer.readyPromise.promise;
	}

	abort(reason) {
		const writer = WritableStreamDefaultWriterBrand.get(this);
		const stream = writer.stream;
		if (stream === undefined)
			return Promise.reject(new TypeError("no stream"));
		return WritableStreamDefaultWriterAbort(writer, reason)		
	}
	close() {
		const writer = WritableStreamDefaultWriterBrand.get(this);
		const stream = writer.stream;
		if (stream === undefined)
			return Promise.reject(new TypeError("no stream"));
		if (WritableStreamCloseQueuedOrInFlight(stream))
			return Promise.reject(new TypeError("stream closed"));
		return WritableStreamDefaultWriterClose(writer);
	}
	releaseLock() {
		const writer = WritableStreamDefaultWriterBrand.get(this);
		const stream = writer.stream;
		if (stream === undefined)
			return;
		assert(stream.writer !== undefined);
		WritableStreamDefaultWriterRelease(writer);
	}
	write(chunk) {
		const writer = WritableStreamDefaultWriterBrand.get(this);
		const stream = writer.stream;
		if (stream === undefined)
			return Promise.reject(new TypeError("no stream"));
		return WritableStreamDefaultWriterWrite(writer, chunk);
	}
};

// 5.4 WritableStreamDefaultController
const WritableStreamDefaultControllerBrand = new Brand("WritableStreamDefaultController");
class WritableStreamDefaultController {
	constructor(token) {
		if (token !== WritableStreamDefaultControllerBrand.token)
			throw new TypeError("new WritableStreamDefaultController");
		const controller = {
			self: this,
			abortSteps: WritableStreamDefaultControllerAbortSteps,
			errorSteps: WritableStreamDefaultControllerErrorSteps,
		};
		WritableStreamDefaultControllerBrand.set(this, controller);
		return controller;
	}
	get signal() {
		const controller = WritableStreamDefaultControllerBrand.get(this);
		return controller.abortController.signal;
	}
	error(e) {
		const controller = WritableStreamDefaultControllerBrand.get(this);
		let state = controller.stream.state;
		if (state != "writable")
			return;
		WritableStreamDefaultControllerError(controller, e);
	}
};
// 5.4.4 Internal Methods
function WritableStreamDefaultControllerAbortSteps(reason) {
	let result = this.abortAlgorithm(reason);
	WritableStreamDefaultControllerClearAlgorithms(this);
	return result;
}
function WritableStreamDefaultControllerErrorSteps() {
	ResetQueue(this);
}

// 5.5.1 Working with writable streams
function CreateWritableStream(startAlgorithm, writeAlgorithm, closeAlgorithm, abortAlgorithm, highWaterMark = 1, sizeAlgorithm = () => 1) {
	assert(IsNonNegativeNumber(highWaterMark) === true);
	const stream = new WritableStream(WritableStreamBrand.token);
	InitializeWritableStream(stream);
	const controller = new WritableStreamDefaultController(WritableStreamDefaultControllerBrand.token);
	SetUpWritableStreamDefaultController(stream, controller, startAlgorithm, writeAlgorithm, closeAlgorithm, abortAlgorithm, highWaterMark, sizeAlgorithm);
	return stream;
}
function AcquireWritableStreamDefaultWriter(stream) {
	return new WritableStreamDefaultWriter(stream.self, WritableStreamDefaultWriterBrand.token);
}
function InitializeWritableStream(stream) {
	stream.backpressure = false;
	stream.closeRequest = undefined;
	stream.controller = undefined;
	stream.inFlightCloseRequest = undefined;
	stream.inFlightWriteRequest = undefined;
	stream.pendingAbortRequest = undefined;
	stream.state = "writable";
	stream.storedError = undefined;
	stream.writeRequests = [];
}
function IsWritableStreamLocked(stream) {
	return stream.writer !== undefined;
}
function SetUpWritableStreamDefaultWriter(writer, stream) {
	if (IsWritableStreamLocked(stream))
		throw new TypeError("writable stream is locked");
	writer.stream = stream;
	stream.writer = writer;
	let state = stream.state;
	if (state == "writable") {
		if ((WritableStreamCloseQueuedOrInFlight(stream) === false) && (stream.backpressure === true))
			writer.readyPromise = createPromise();
		else
			writer.readyPromise = createResolvedPromise(undefined);
		writer.closedPromise = createPromise();
	}
	else if (state == "erroring") {
		writer.readyPromise = createRejectedPromise(stream.storedError);
		handlePromise(writer.readyPromise);
		writer.closedPromise = createPromise();
	}
	else if (state == "closed") {
		writer.readyPromise = createResolvedPromise(undefined);
		writer.closedPromise = createResolvedPromise(undefined);
	}
	else {
		assert(state == "errored");
		let storedError = stream.storedError;
		writer.readyPromise = createRejectedPromise(storedError);
		handlePromise(writer.readyPromise);
		writer.closedPromise = createRejectedPromise(storedError);
		handlePromise(writer.closedPromise);
	}
}
function WritableStreamAbort(stream, reason) {
	let state = stream.state;
	if ((state == "closed") || (state == "errored")) {
		return Promise.resolve(undefined);
	}
	stream.controller.abortController.abort(reason);
	state = stream.state;
	if ((state == "closed") || (state == "errored")) {
		return Promise.resolve(undefined);
	}
	if (stream.pendingAbortRequest !== undefined)
		return stream.pendingAbortRequest.promise;
	assert((state == "writable") || (state == "erroring"));
	let wasAlreadyErroring = false;
	if (state == "erroring") {
		wasAlreadyErroring = true;
		reason = undefined;
	}
	let pendingAbortRequest = createPromise();
	pendingAbortRequest.reason = reason; 
	pendingAbortRequest.wasAlreadyErroring = wasAlreadyErroring; 
	stream.pendingAbortRequest = pendingAbortRequest 
	if (wasAlreadyErroring === false)
		WritableStreamStartErroring(stream, reason);
	return pendingAbortRequest.promise;
}
function WritableStreamClose(stream) {
	let state = stream.state;
	if ((state == "closed") || (state == "errored"))
		return Promise.reject(new TypeError("stream already closed or errored"));
	assert((state == "writable") || (state == "erroring"));
	assert(WritableStreamCloseQueuedOrInFlight(stream) === false);
	let closeRequest = createPromise();
	stream.closeRequest = closeRequest;
	let writer = stream.writer;
	if ((writer !== undefined) && stream.backpressure && (state == "writable"))
		resolvePromise(writer.readyPromise, undefined);
	WritableStreamDefaultControllerClose(stream.controller);
	return closeRequest.promise;
}

// 5.5.2 Interfacing with controllers
function WritableStreamAddWriteRequest(stream) {
	assert(IsWritableStreamLocked(stream) == true);
	let state = stream.state;	
	assert(state == "writable");
	let writeRequest = createPromise();
	stream.writeRequests.push(writeRequest);
	return writeRequest;
}
function WritableStreamCloseQueuedOrInFlight(stream) {
	return (stream.closeRequest !== undefined) || (stream.inFlightCloseRequest !== undefined);
}
function WritableStreamDealWithRejection(stream, error) {
	let state = stream.state;	
	if (state == "writable") {
		WritableStreamStartErroring(stream, error);
		return;
	}
	assert(state == "erroring");
	WritableStreamFinishErroring(stream);
}	
function WritableStreamFinishErroring(stream) {
	let state = stream.state;	
	assert(state == "erroring");
	stream.state = "errored";
	stream.controller.errorSteps();
	let storedError = stream.storedError;
	stream.writeRequests.forEach(writeRequest => {
		rejectPromise(writeRequest, storedError);
	});
	stream.writeRequests = [];
	if (stream.pendingAbortRequest === undefined) {
		WritableStreamRejectCloseAndClosedPromiseIfNeeded(stream);
		return;
	}
	let pendingAbortRequest = stream.pendingAbortRequest;
	stream.pendingAbortRequest = undefined;
	if (pendingAbortRequest.wasAlreadyErroring) {
		rejectPromise(pendingAbortRequest, storedError);
		WritableStreamRejectCloseAndClosedPromiseIfNeeded(stream);
		return;
	}
	stream.controller.abortSteps(storedError).then(value => {
		resolvePromise(pendingAbortRequest, undefined);
		WritableStreamRejectCloseAndClosedPromiseIfNeeded(stream);
	}, reason => {
		rejectPromise(pendingAbortRequest, reason);
		WritableStreamRejectCloseAndClosedPromiseIfNeeded(stream);
	});
}
function WritableStreamFinishInFlightClose(stream) {
	assert(stream.inFlightCloseRequest !== undefined);
	resolvePromise(stream.inFlightCloseRequest, undefined);
	stream.inFlightCloseRequest = undefined;
	let state = stream.state;	
	assert((state == "writable") || (state == "erroring"));
	if (state == "erroring") {
		stream.storedError = undefined;
		if (stream.pendingAbortRequest !== undefined) {
			resolvePromise(stream.pendingAbortRequest, undefined);
			stream.pendingAbortRequest = undefined;
		}
	}
	stream.state = "closed";
	let writer = stream.writer;
	if (writer !==	undefined) {
		resolvePromise(writer.closedPromise, undefined);
	}
	assert(stream.pendingAbortRequest === undefined);
	assert(stream.storedError === undefined);
}
function WritableStreamFinishInFlightCloseWithError(stream, error) {
	assert(stream.inFlightCloseRequest !== undefined);
	rejectPromise(stream.inFlightCloseRequest, error);
	stream.inFlightCloseRequest = undefined;
	let state = stream.state;	
	assert((state == "writable") || (state == "erroring"));
	if (stream.pendingAbortRequest !== undefined) {
		rejectPromise(stream.pendingAbortRequest, error);
		stream.pendingAbortRequest = undefined;
	}
	WritableStreamDealWithRejection(stream, error);
}
function WritableStreamFinishInFlightWrite(stream) {
	assert(stream.inFlightWriteRequest !== undefined);
	resolvePromise(stream.inFlightWriteRequest, undefined);
	stream.inFlightWriteRequest = undefined;
}
function WritableStreamFinishInFlightWriteWithError(stream, error) {
	assert(stream.inFlightWriteRequest !== undefined);
	rejectPromise(stream.inFlightWriteRequest, error);
	stream.inFlightWriteRequest = undefined;
	let state = stream.state;	
	assert((state == "writable") || (state == "erroring"));
	WritableStreamDealWithRejection(stream, error);
}
function WritableStreamHasOperationMarkedInFlight(stream) {
	return (stream.inFlightWriteRequest !== undefined) || (stream.inFlightCloseRequest !== undefined);
}
function WritableStreamMarkCloseRequestInFlight(stream) {
	assert(stream.inFlightCloseRequest === undefined);
	assert(stream.closeRequest !== undefined);
	stream.inFlightCloseRequest = stream.closeRequest;
	stream.closeRequest = undefined;
}
function WritableStreamMarkFirstWriteRequestInFlight(stream) {
	assert(stream.inFlightWriteRequest === undefined);
	assert(stream.writeRequests.length > 0);
	stream.inFlightWriteRequest = stream.writeRequests.shift();
}
function WritableStreamRejectCloseAndClosedPromiseIfNeeded(stream) {
	assert(stream.state == "errored");
	if (stream.closeRequest !==	undefined) {
		assert(stream.inFlightCloseRequest ===	undefined);
		rejectPromise(stream.closeRequest, stream.storedError);
		stream.closeRequest = undefined;
	}
	let writer = stream.writer;
	if (writer !==	undefined) {
		rejectPromise(writer.closedPromise, stream.storedError);
		handlePromise(writer.closedPromise);
	}
}
function WritableStreamStartErroring(stream, reason) {
	assert(stream.storedError === undefined);
	assert(stream.state == "writable");
	let controller = stream.controller;
	assert(controller !== undefined);
	stream.state = "erroring";
	stream.storedError = reason;
	let writer = stream.writer;
	if (writer !==	undefined)
		WritableStreamDefaultWriterEnsureReadyPromiseRejected(writer, reason);
	if ((WritableStreamHasOperationMarkedInFlight(stream) === false) && (controller.started === true))
		WritableStreamFinishErroring(stream);
		
}
function WritableStreamUpdateBackpressure(stream, backpressure) {
	assert(stream.state == "writable");
	assert(WritableStreamCloseQueuedOrInFlight(stream) == false);
	let writer = stream.writer;
	if ((writer !== undefined) && (stream.backpressure != backpressure)) {
		if (backpressure)
			writer.readyPromise = createPromise();
		else 
			resolvePromise(writer.readyPromise, undefined);
	}
	stream.backpressure = backpressure;
}

// 5.5.3 Writers
function WritableStreamDefaultWriterAbort(writer, reason) {
	let stream = writer.stream;
	assert(stream !== undefined);
	return WritableStreamAbort(stream, reason);
}
function WritableStreamDefaultWriterClose(writer) {
	let stream = writer.stream;
	assert(stream !== undefined);
	return WritableStreamClose(stream);
}
function WritableStreamDefaultWriterCloseWithErrorPropagation(writer) {
	let stream = writer.stream;
	assert(stream !== undefined);
	let state = stream.state;
	if (WritableStreamCloseQueuedOrInFlight(stream) || (state == "closed"))
		return Promise.resolve(undefined);
	if (state == "errored")
		return Promise.reject(stream.storedError);
	assert((state == "writable") || (state == "erroring"));
	return WritableStreamDefaultWriterClose(writer);
}
function WritableStreamDefaultWriterEnsureClosedPromiseRejected(writer, error) {
	if (isPromisePending(writer.closedPromise))
		rejectPromise(writer.closedPromise, error);
	else
		writer.closedPromise = createRejectedPromise(error);
	handlePromise(writer.closedPromise);
}
function WritableStreamDefaultWriterEnsureReadyPromiseRejected(writer, error) {
	if (isPromisePending(writer.readyPromise))
		rejectPromise(writer.readyPromise, error);
	else
		writer.readyPromise = createRejectedPromise(error);
	handlePromise(writer.readyPromise);
}
function WritableStreamDefaultWriterGetDesiredSize(writer) {
	let stream = writer.stream;
	let state = stream.state;
	if ((state == "errored") || (state == "erroring"))
		return null;
	if (state == "closed")
		return 0;
	return WritableStreamDefaultControllerGetDesiredSize(stream.controller);
}
function WritableStreamDefaultWriterRelease(writer) {
	let stream = writer.stream;
	assert(stream !== undefined);
	assert(stream.writer === writer);
	let error = new TypeError("writer released");
	WritableStreamDefaultWriterEnsureReadyPromiseRejected(writer, error);
	WritableStreamDefaultWriterEnsureClosedPromiseRejected(writer, error);
	stream.writer = undefined;
	writer.stream = undefined;
}
function WritableStreamDefaultWriterWrite(writer, chunk) {
	let stream = writer.stream;
	assert(stream !== undefined);
	let controller = stream.controller;
	let chunkSize = WritableStreamDefaultControllerGetChunkSize(controller, chunk);
	if (stream != writer.stream)
		return Promise.reject(new TypeError("???"));
	let state = stream.state;
	if (state == "errored")
		return Promise.reject(stream.storedError);
	if (WritableStreamCloseQueuedOrInFlight(stream) || (state == "closed"))
		return Promise.reject(new TypeError("stream closing or closed"));
	if (state == "erroring")
		return Promise.reject(stream.storedError);
	assert(state == "writable")
	let request = WritableStreamAddWriteRequest(stream);
	WritableStreamDefaultControllerWrite(controller, chunk, chunkSize);
	return request.promise;
}

// 5.5.4 Default controllers
function SetUpWritableStreamDefaultController(stream, controller, startAlgorithm, writeAlgorithm, closeAlgorithm, abortAlgorithm, highWaterMark, sizeAlgorithm) {
	assert(stream.controller === undefined);
	controller.stream = stream;
	stream.controller = controller;
	controller.queue = undefined;
	controller.queueTotalSize = undefined;
	ResetQueue(controller);
	controller.abortController = new AbortController;
	controller.started = false;
	controller.strategySizeAlgorithm = sizeAlgorithm;
	controller.strategyHWM = highWaterMark;
	controller.writeAlgorithm = writeAlgorithm;
	controller.closeAlgorithm = closeAlgorithm;
	controller.abortAlgorithm = abortAlgorithm;
	let backpressure =	WritableStreamDefaultControllerGetBackpressure(controller);
	WritableStreamUpdateBackpressure(stream, backpressure);
	const startResult = startAlgorithm(controller.self);
	const startPromise = Promise.resolve(startResult);
	startPromise.then(
		value => {
			let state = stream.state;
			assert((state == "writable") || (state == "erroring"));
			controller.started = true;
			WritableStreamDefaultControllerAdvanceQueueIfNeeded(controller);
		},
		reason => {
			let state = stream.state;
			assert((state == "writable") || (state == "erroring"));
			controller.started = true;
			WritableStreamDealWithRejection(stream, reason);
		}
	);
}
function SetUpWritableStreamDefaultControllerFromUnderlyingSink(stream, underlyingSink, underlyingSinkDict, highWaterMark, sizeAlgorithm) {
	const controller = new WritableStreamDefaultController(WritableStreamDefaultControllerBrand.token);
	const startAlgorithm = createCallback(underlyingSinkDict.start, underlyingSink);
	const writeAlgorithm = createPromiseCallback(underlyingSinkDict.write, underlyingSink);
	const closeAlgorithm = createPromiseCallback(underlyingSinkDict.close, underlyingSink);
	const abortAlgorithm = createPromiseCallback(underlyingSinkDict.abort, underlyingSink);
	SetUpWritableStreamDefaultController(stream, controller, startAlgorithm, writeAlgorithm, closeAlgorithm, abortAlgorithm, highWaterMark, sizeAlgorithm);
}
const closeSentinel = Symbol('close sentinel');
function WritableStreamDefaultControllerAdvanceQueueIfNeeded(controller) {
	let stream = controller.stream;
	if (!controller.started)
		return;
	if (stream.inFlightWriteRequest !== undefined)
		return;
	let state = stream.state;
	assert((state != "closed") && (state != "errored"));
	if (state == "erroring") {
		WritableStreamFinishErroring(stream);
		return;
	}
	if (controller.queue.length == 0)
		return;
	let value = PeekQueueValue(controller);
	if (value === closeSentinel)
		WritableStreamDefaultControllerProcessClose(controller);
	else
		WritableStreamDefaultControllerProcessWrite(controller, value);
}
function WritableStreamDefaultControllerClearAlgorithms(controller) {
	controller.strategySizeAlgorithm = undefined;
	controller.writeAlgorithm = undefined;
	controller.closeAlgorithm = undefined;
	controller.abortAlgorithm = undefined;
}
function WritableStreamDefaultControllerClose(controller) {
	EnqueueValueWithSize(controller, closeSentinel, 0);
	WritableStreamDefaultControllerAdvanceQueueIfNeeded(controller);
}
function WritableStreamDefaultControllerError(controller, error) {
	let stream = controller.stream;
	assert(stream.state == "writable");
	WritableStreamDefaultControllerClearAlgorithms(controller);
	WritableStreamStartErroring(stream, error);
}
function WritableStreamDefaultControllerErrorIfNeeded(controller, error) {
	let stream = controller.stream;
	if (stream.state == "writable")
		WritableStreamDefaultControllerError(controller, error);
}
function WritableStreamDefaultControllerGetBackpressure(controller) {
	let desiredSize = WritableStreamDefaultControllerGetDesiredSize(controller);
	return (desiredSize <= 0);
}
function WritableStreamDefaultControllerGetChunkSize(controller, chunk) {
	let result = 1;
	try {
		result = controller.strategySizeAlgorithm(chunk);
	}
	catch(error) {
		WritableStreamDefaultControllerErrorIfNeeded(controller, error);
	}
	return result;
}
function WritableStreamDefaultControllerGetDesiredSize(controller) {
	return controller.strategyHWM - controller.queueTotalSize;
}
function WritableStreamDefaultControllerProcessClose(controller) {
	let stream = controller.stream;
	WritableStreamMarkCloseRequestInFlight(stream);
	DequeueValue(controller);
	assert(controller.queue.length === 0);
	let sinkClosePromise = controller.closeAlgorithm();
	WritableStreamDefaultControllerClearAlgorithms(controller);
	sinkClosePromise.then(
		value => {
			WritableStreamFinishInFlightClose(stream);
		},
		reason => {
			WritableStreamFinishInFlightCloseWithError(stream, reason);
		}
	);
}
function WritableStreamDefaultControllerProcessWrite(controller, chunk) {
	let stream = controller.stream;
	WritableStreamMarkFirstWriteRequestInFlight(stream);
	let sinkWritePromise = controller.writeAlgorithm(chunk, controller.self);
	sinkWritePromise.then(
		value => {
			WritableStreamFinishInFlightWrite(stream);
			let state = stream.state;
			assert((state == "writable") || (state == "erroring"));
			DequeueValue(controller);
			if (!WritableStreamCloseQueuedOrInFlight(stream) && (state == "writable")) {
				let backpressure = WritableStreamDefaultControllerGetBackpressure(controller);
				WritableStreamUpdateBackpressure(stream, backpressure);
			}
			WritableStreamDefaultControllerAdvanceQueueIfNeeded(controller);
		},
		reason => {
			if (stream.state == "writable")
				WritableStreamDefaultControllerClearAlgorithms(controller);
			WritableStreamFinishInFlightWriteWithError(stream, reason);
		}
	);
}
function WritableStreamDefaultControllerWrite(controller, chunk, chunkSize) {
	try {
		let enqueueResult = EnqueueValueWithSize(controller, chunk, chunkSize);
	}
	catch(error) {
		WritableStreamDefaultControllerErrorIfNeeded(controller, error);
		return;
	}
	let stream = controller.stream;
	let state = stream.state;
	if (!WritableStreamCloseQueuedOrInFlight(stream) && (state == "writable")) {
		let backpressure = WritableStreamDefaultControllerGetBackpressure(controller);
		WritableStreamUpdateBackpressure(stream, backpressure);
	}
	WritableStreamDefaultControllerAdvanceQueueIfNeeded(controller);
}

// 6.2 TransformStream
const TransformStreamBrand = new Brand("TransformStream");
class TransformStream {
	constructor(transformer, writableStrategy, readableStrategy) {
		const stream = { self:this };
		TransformStreamBrand.set(this, stream);
		if (transformer === undefined) {
			transformer = null;
		}
		const transformerDict = transformer ?? {};
		if ('readableType' in transformerDict) {
			throw new RangeError('Invalid readableType specified');
		}
		if ('writableType' in transformerDict) {
			throw new RangeError('Invalid writableType specified');
		}

		const readableHighWaterMark = ExtractHighWaterMark(readableStrategy, 0);
		const readableSizeAlgorithm = ExtractSizeAlgorithm(readableStrategy);
		const writableHighWaterMark = ExtractHighWaterMark(writableStrategy, 1);
		const writableSizeAlgorithm = ExtractSizeAlgorithm(writableStrategy);

		const startPromise = createPromise();

		InitializeTransformStream(
			stream, startPromise.promise, writableHighWaterMark, writableSizeAlgorithm, readableHighWaterMark, readableSizeAlgorithm
		);
		SetUpTransformStreamDefaultControllerFromTransformer(stream, transformer, transformerDict);

		const startAlgorithm = createCallback(transformerDict.start, transformer);
		const startResult = startAlgorithm(stream.controller.self);
		resolvePromise(startPromise, startResult);
	}
	get readable() {
		const stream = TransformStreamBrand.get(this);
		return stream.readable.self;
	}
	get writable() {
		const stream = TransformStreamBrand.get(this);
		return stream.writable.self;
	}
}

// 6.3 TransformStreamDefaultController
const TransformStreamDefaultControllerBrand = new Brand("TransformStreamDefaultController");
class TransformStreamDefaultController {
	constructor(token) {
		if (token !== TransformStreamDefaultControllerBrand.token)
			throw new TypeError("new TransformStreamDefaultController");
		const controller = { 
			self: this,
			cancelAlgorithm: null,
			flushAlgorithm: null,
			stream: null,
			transformAlgorithm: null,
		};
		TransformStreamDefaultControllerBrand.set(this, controller);
		return controller;
	}
	get desiredSize() {
		const controller = TransformStreamDefaultControllerBrand.get(this);
		const readableController = controller.stream.readable.controller;
		return ReadableStreamDefaultControllerGetDesiredSize(readableController);
	}
	enqueue(chunk) {
		const controller = TransformStreamDefaultControllerBrand.get(this);
		TransformStreamDefaultControllerEnqueue(controller, chunk);
	}
	error(reason) {
		const controller = TransformStreamDefaultControllerBrand.get(this);
		TransformStreamDefaultControllerError(controller, reason);
	}
	terminate() {
		const controller = TransformStreamDefaultControllerBrand.get(this);
		TransformStreamDefaultControllerTerminate(controller);
	}
}

// 6.4.1 Working with transform streams
function InitializeTransformStream(
	stream, startPromise, writableHighWaterMark, writableSizeAlgorithm, readableHighWaterMark, readableSizeAlgorithm) {
	function startAlgorithm() {
		return startPromise;
	}

	function writeAlgorithm(chunk) {
		return TransformStreamDefaultSinkWriteAlgorithm(stream, chunk);
	}

	function abortAlgorithm(reason) {
		return TransformStreamDefaultSinkAbortAlgorithm(stream, reason);
	}

	function closeAlgorithm() {
		return TransformStreamDefaultSinkCloseAlgorithm(stream);
	}

	stream.writable = CreateWritableStream(
		startAlgorithm, writeAlgorithm, closeAlgorithm, abortAlgorithm, writableHighWaterMark, writableSizeAlgorithm
	);

	function pullAlgorithm() {
		return TransformStreamDefaultSourcePullAlgorithm(stream);
	}

	function cancelAlgorithm(reason) {
		return TransformStreamDefaultSourceCancelAlgorithm(stream, reason);
	}

	stream.readable = CreateReadableStream(
		startAlgorithm, pullAlgorithm, cancelAlgorithm, readableHighWaterMark, readableSizeAlgorithm
	);

	// The [[backpressure]] slot is set to undefined so that it can be initialised by TransformStreamSetBackpressure.
	stream.backpressure = undefined;
	stream.backpressureChangePromise = undefined;
	TransformStreamSetBackpressure(stream, true);

	stream.controller = undefined;
}
function TransformStreamError(stream, e) {
	verbose('TransformStreamError()');

	ReadableStreamDefaultControllerError(stream.readable.controller, e);
	TransformStreamErrorWritableAndUnblockWrite(stream, e);
}
function TransformStreamErrorWritableAndUnblockWrite(stream, e) {
	TransformStreamDefaultControllerClearAlgorithms(stream.controller);
	WritableStreamDefaultControllerErrorIfNeeded(stream.writable.controller, e);
	TransformStreamUnblockWrite(stream);
}
function TransformStreamUnblockWrite(stream) {
	if (stream.backpressure === true) {
		// Pretend that pull() was called to permit any pending write() calls to complete. TransformStreamSetBackpressure()
		// cannot be called from enqueue() or pull() once the ReadableStream is errored, so this will will be the final time
		// _backpressure is set.
		TransformStreamSetBackpressure(stream, false);
	}
}
function TransformStreamSetBackpressure(stream, backpressure) {
	verbose(`TransformStreamSetBackpressure() [backpressure = ${backpressure}]`);

	// Passes also when called during construction.
	assert(stream.backpressure !== backpressure);

	if (stream.backpressureChangePromise !== undefined) {
		resolvePromise(stream.backpressureChangePromise, undefined);
	}

	stream.backpressureChangePromise = createPromise();

	stream.backpressure = backpressure;
}

// 6.4.2 Default controllers
function SetUpTransformStreamDefaultController(stream, controller, transformAlgorithm, flushAlgorithm, cancelAlgorithm) {
	assert(TransformStreamBrand.has(stream.self));
	assert(stream.controller === undefined);

	controller.stream = stream;
	stream.controller = controller;

	controller.transformAlgorithm = transformAlgorithm;
	controller.flushAlgorithm = flushAlgorithm;
	controller.cancelAlgorithm = cancelAlgorithm;
}
function SetUpTransformStreamDefaultControllerFromTransformer(stream, transformer, transformerDict) {
	const controller = new TransformStreamDefaultController(TransformStreamDefaultControllerBrand.token);
	const transformAlgorithm = createPromiseCallback(transformerDict.transform, transformer, chunk => {
		try {
			TransformStreamDefaultControllerEnqueue(controller, chunk);
			return Promise.resolve(undefined);
		} catch (e) {
			return Promise.reject(e);
		}
	});
	const flushAlgorithm = createPromiseCallback(transformerDict.flush, transformer);
	const cancelAlgorithm = createPromiseCallback(transformerDict.cancel, transformer);
	SetUpTransformStreamDefaultController(stream, controller, transformAlgorithm, flushAlgorithm, cancelAlgorithm);
}
function TransformStreamDefaultControllerClearAlgorithms(controller) {
	controller.transformAlgorithm = undefined;
	controller.flushAlgorithm = undefined;
	controller.cancelAlgorithm = undefined;
}
function TransformStreamDefaultControllerEnqueue(controller, chunk) {
	verbose('TransformStreamDefaultControllerEnqueue()');

	const stream = controller.stream;
	const readableController = stream.readable.controller;
	if (ReadableStreamDefaultControllerCanCloseOrEnqueue(readableController) === false) {
		throw new TypeError('Readable side is not in a state that permits enqueue');
	}

	// We throttle transform invocations based on the backpressure of the ReadableStream, but we still
	// accept TransformStreamDefaultControllerEnqueue() calls.

	try {
		ReadableStreamDefaultControllerEnqueue(readableController, chunk);
	} catch (e) {
		// This happens when readableStrategy.size() throws.
		TransformStreamErrorWritableAndUnblockWrite(stream, e);

		throw stream.readable.storedError;
	}

	const backpressure = ReadableStreamDefaultControllerHasBackpressure(readableController);
	if (backpressure !== stream.backpressure) {
		assert(backpressure === true);
		TransformStreamSetBackpressure(stream, true);
	}
}
function TransformStreamDefaultControllerError(controller, e) {
	TransformStreamError(controller.stream, e);
}
function TransformStreamDefaultControllerPerformTransform(controller, chunk) {
	const transformPromise = controller.transformAlgorithm(chunk, controller.self);
	return transformPromise.catch(r => {
		TransformStreamError(controller.stream, r);
		throw r;
	});
}
function TransformStreamDefaultControllerTerminate(controller) {
	verbose('TransformStreamDefaultControllerTerminate()');

	const stream = controller.stream;
	const readableController = stream.readable.controller;

	ReadableStreamDefaultControllerClose(readableController);

	const error = new TypeError('TransformStream terminated');
	TransformStreamErrorWritableAndUnblockWrite(stream, error);
}

// 6.4.3 Default sinks
function TransformStreamDefaultSinkWriteAlgorithm(stream, chunk) {
	verbose('TransformStreamDefaultSinkWriteAlgorithm()');

	assert(stream.writable.state === 'writable');

	const controller = stream.controller;

	if (stream.backpressure === true) {
		const backpressureChangePromise = stream.backpressureChangePromise;
		assert(backpressureChangePromise !== undefined);
		return backpressureChangePromise.promise.then(() => {
			const writable = stream.writable;
			const state = writable.state;
			if (state === 'erroring') {
				throw writable.storedError;
			}
			assert(state === 'writable');
			return TransformStreamDefaultControllerPerformTransform(controller, chunk);
		});
	}

	return TransformStreamDefaultControllerPerformTransform(controller, chunk);
}
function TransformStreamDefaultSinkAbortAlgorithm(stream, reason) {
	verbose('TransformStreamDefaultSinkAbortAlgorithm()');

	const controller = stream.controller;
	if (controller.finishPromise !== undefined) {
		return controller.finishPromise.promise;
	}

	// stream.readable cannot change after construction, so caching it across a call to user code is safe.
	const readable = stream.readable;

	// Assign the _finishPromise now so that if _cancelAlgorithm calls readable.cancel() internally,
	// we don't run the _cancelAlgorithm again.
	controller.finishPromise = createPromise();

	const cancelPromise = controller.cancelAlgorithm(reason);
	TransformStreamDefaultControllerClearAlgorithms(controller);

	cancelPromise.then(v => {
		if (readable.state === 'errored') {
			rejectPromise(controller.finishPromise, readable.storedError);
		} else {
			ReadableStreamDefaultControllerError(readable.controller, reason);
			resolvePromise(controller.finishPromise);
		}
	}, r => {
		ReadableStreamDefaultControllerError(readable.controller, r);
		rejectPromise(controller.finishPromise, r);
	});

	return controller.finishPromise.promise;
}
function TransformStreamDefaultSinkCloseAlgorithm(stream) {
	verbose('TransformStreamDefaultSinkCloseAlgorithm()');

	const controller = stream.controller;
	if (controller.finishPromise !== undefined) {
		return controller.finishPromise.promise;
	}

	// stream.readable cannot change after construction, so caching it across a call to user code is safe.
	const readable = stream.readable;

	// Assign the _finishPromise now so that if _flushAlgorithm calls readable.cancel() internally,
	// we don't also run the _cancelAlgorithm.
	controller.finishPromise = createPromise();

	const flushPromise = controller.flushAlgorithm(controller.self);
	TransformStreamDefaultControllerClearAlgorithms(controller);

	flushPromise.then(() => {
		if (readable.state === 'errored') {
			rejectPromise(controller.finishPromise, readable.storedError);
		} else {
			ReadableStreamDefaultControllerClose(readable.controller);
			resolvePromise(controller.finishPromise);
		}
	}, r => {
		ReadableStreamDefaultControllerError(readable.controller, r);
		rejectPromise(controller.finishPromise, r);
	});

	return controller.finishPromise.promise;
}

// 6.4.4 Default sources
function TransformStreamDefaultSourcePullAlgorithm(stream) {
	verbose('TransformStreamDefaultSourcePullAlgorithm()');

	// Invariant. Enforced by the promises returned by start() and pull().
	assert(stream.backpressure === true);

	assert(stream.backpressureChangePromise !== undefined);

	TransformStreamSetBackpressure(stream, false);

	// Prevent the next pull() call until there is backpressure.
	return stream.backpressureChangePromise.promise;
}
function TransformStreamDefaultSourceCancelAlgorithm(stream, reason) {
	verbose('TransformStreamDefaultSourceCancelAlgorithm()');

	const controller = stream.controller;
	if (controller.finishPromise !== undefined) {
		return controller.finishPromise.promise;
	}

	// stream.writable cannot change after construction, so caching it across a call to user code is safe.
	const writable = stream.writable;

	// Assign the _finishPromise now so that if _flushAlgorithm calls writable.abort() or
	// writable.cancel() internally, we don't run the _cancelAlgorithm again, or also run the
	// _flushAlgorithm.
	controller.finishPromise = createPromise();

	const cancelPromise = controller.cancelAlgorithm(reason);
	TransformStreamDefaultControllerClearAlgorithms(controller);

	cancelPromise.then(() => {
		if (writable.state === 'errored') {
			rejectPromise(controller.finishPromise, writable.storedError);
		} else {
			WritableStreamDefaultControllerErrorIfNeeded(writable.controller, reason);
			TransformStreamUnblockWrite(stream);
			resolvePromise(controller.finishPromise);
		}
	}, r => {
		WritableStreamDefaultControllerErrorIfNeeded(writable.controller, r);
		TransformStreamUnblockWrite(stream);
		rejectPromise(controller.finishPromise, r);
	});

	return controller.finishPromise.promise;
}

// 7.2 ByteLengthQueuingStrategy
class ByteLengthQueuingStrategy {
	#highWaterMark;
	constructor(init) {
		const highWaterMark = init?.highWaterMark;
		if (highWaterMark === undefined)
			throw new TypeError("invalid highWaterMark");
		this.#highWaterMark = Number(highWaterMark);
	}
	get highWaterMark() {
		return this.#highWaterMark;
	}
	size(chunk) {
		return chunk.byteLength;
	}
}

// 7.3 CountQueuingStrategy
class CountQueuingStrategy {
	#highWaterMark;
	constructor(init) {
		const highWaterMark = init?.highWaterMark;
		if (highWaterMark === undefined)
			throw new TypeError("invalid highWaterMark");
		this.#highWaterMark = Number(highWaterMark);
	}
	get highWaterMark() {
		return this.#highWaterMark;
	}
	size() {
		return 1;
	}
}

// 7.4 Abstract operations
function ExtractHighWaterMark(strategy, defaultHWM) {
	let highWaterMark = defaultHWM;
	if (strategy && ("highWaterMark" in strategy))
		highWaterMark = Number(strategy.highWaterMark);
	if (isNaN(highWaterMark) || (highWaterMark < 0))
		throw new RangeError("invalid highWaterMark");
	return highWaterMark;
}
function ExtractSizeAlgorithm(strategy) {
	let size, sizeAlgorithm;
	if (strategy && ("size" in strategy))
		size = strategy.size;
	if (size) {
		if (typeof(size) !== "function")
			throw new TypeError("size is no function");
		sizeAlgorithm = (chunk) => {
			return size(chunk);
		};
	}
	else
		sizeAlgorithm = (chunk) => {
			return 1;
		};
	return sizeAlgorithm;
}

// 8.1 Queue-with-sizes
function DequeueValue(container)	{
	assert(container.queue.length > 0);
	let valueWithSize = container.queue.shift();
	container.queueTotalSize -= valueWithSize.size;
	if (container.queueTotalSize < 0)
		container.queueTotalSize = 0;
	return valueWithSize.value;
}
function EnqueueValueWithSize(container, value, size) {
	if (!IsNonNegativeNumber(size))
		throw new RangeError("invalid size");
	if (size == Number.POSITIVE_INFINITY)
		throw new RangeError("invalid size");
	container.queue.push({ value, size });
	container.queueTotalSize += size;
}
function PeekQueueValue(container) {
	assert(container.queue.length > 0);
	let valueWithSize = container.queue[0];
	return valueWithSize.value;
}
function ResetQueue(container) {
	container.queue = [];
	container.queueTotalSize = 0;
}

// 8.3 Miscellaneous

function IsNonNegativeNumber(v) {
	if (typeof(v) != "number")
		return false;
	if (isNaN(v))
		return false;
	return v >= 0;
}

class TextDecoderStream extends TransformStream {
	#decoder;
	constructor(label="utf-8", options={}) {
		const decoder = new TextDecoder(label, options);
		const decodeOptions = { stream:true };
		super({
			transform(chunk, controller) {
				if (!(ArrayBuffer.isView(chunk) || (chunk instanceof ArrayBuffer) || (chunk instanceof SharedArrayBuffer)))
					throw new TypeError("invalid chunk");
				if (chunk.byteLength > 0) {
					const string = decoder.decode(chunk, decodeOptions);
					if (string.length)
    					controller.enqueue(string);
    			}
			},
			flush(controller) {
				const string = decoder.decode();
				if (string.length)
					controller.enqueue(string);
			}
		});
		this.#decoder = decoder;
	}
	get encoding() {
		return this.#decoder.encoding;
	}
	get fatal() {
		return this.#decoder.fatal;
	}
	get ignoreBOM() {
		return this.#decoder.ignoreBOM;
	}
};

const replacement = String.fromCharCode(0xFFFD);
class TextEncoderStream extends TransformStream {
	constructor() {
		const encoder = new TextEncoder();
		let pendingSurrogate = null;
		super({
			transform(string, controller) {
				string = String(string);
				let length = string.length;
				let result = "";
				for (let i = 0; i < length; i++) {
					const code = string.charCodeAt(i);
					if ((0xD800 <= code) && (code <= 0xDBFF)) {
						if (pendingSurrogate) {
							result += replacement;
							pendingSurrogate = null;
						}
						pendingSurrogate = string[i];
					}
					else if ((0xDC00 <= code) && (code <= 0xDFFF)) {
						if (pendingSurrogate) {
							result += pendingSurrogate + string[i];
							pendingSurrogate = null;
						}
						else {
							result += replacement;
						}
					}
					else {
						if (pendingSurrogate) {
							result += replacement;
							pendingSurrogate = null;
						}
						result += string[i];
					}
				}	
				if (result.length > 0) {
					const chunk = encoder.encode(result);
    				controller.enqueue(chunk);
				}	
			},
			flush(controller) {
				if (pendingSurrogate) {
					const chunk = encoder.encode(replacement);
    				controller.enqueue(chunk);
				}	
			}
		});
	}
};

export {
	queueMicrotask,
	AbortController,
	AbortSignal,
	ByteLengthQueuingStrategy,
	CountQueuingStrategy,
	DOMException,
	ReadableByteStreamController,
	ReadableStream,
	ReadableStreamBYOBReader,
	ReadableStreamBYOBRequest,
	ReadableStreamDefaultController,
	ReadableStreamDefaultReader,
	TransformStream,
	TransformStreamDefaultController,
	WritableStream,
	WritableStreamDefaultController,
	WritableStreamDefaultWriter,
	TextDecoderStream,
	TextEncoderStream,
};

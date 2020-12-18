interface Trace {
	(...log: (string | number | boolean)[]):void;
	left(log: string | ArrayBuffer, conversation?: string):void;
	center(log: string | ArrayBuffer, conversation?: string):void;
	right(log: string | ArrayBuffer, conversation?: string):void;
}
declare const trace:Trace;

declare class HostBuffer {
	readonly byteLength: number
	private brand: boolean;
}

interface ObjectConstructor {
	freeze<T>(obj: T, freeze?: boolean | number): Readonly<T>;
}

interface JSON {
	parse(text: string, reviver?: string[]): any;
}

interface StringConstructor {
	fromArrayBuffer(buffer: ArrayBuffer): string;
}

interface ArrayBufferConstructor {
	fromString(string: string): ArrayBuffer;
}

interface Uint8ArrayConstructor {
	new(buffer: HostBuffer, byteOffset?: number, length?: number): Uint8Array
}

interface Uint8ClampedArrayConstructor {
	new(buffer: HostBuffer, byteOffset?: number, length?: number): Uint8ClampedArrayConstructor
}

interface Uint16ArrayConstructor {
	new(buffer: HostBuffer, byteOffset?: number, length?: number): Uint16Array
}

interface Uint32ArrayConstructor {
	new(buffer: HostBuffer, byteOffset?: number, length?: number): Uint32Array
}

interface Int8ArrayConstructor {
	new(buffer: HostBuffer, byteOffset?: number, length?: number): Int8Array
}

interface Int16ArrayConstructor {
	new(buffer: HostBuffer, byteOffset?: number, length?: number): Int16Array
}

interface Int32ArrayConstructor {
	new(buffer: HostBuffer, byteOffset?: number, length?: number): Uint32Array
}

interface Float32ArrayConstructor {
	new(buffer: HostBuffer, byteOffset?: number, length?: number): Float32Array
}

interface Float64ArrayConstructor {
	new(buffer: HostBuffer, byteOffset?: number, length?: number): Float64Array
}

interface BigInt64ArrayConstructor {
	new(buffer: HostBuffer, byteOffset?: number, length?: number): BigInt64ArrayConstructor
}

interface BigUint64ArrayConstructor {
	new(buffer: HostBuffer, byteOffset?: number, length?: number): BigUint64ArrayConstructor
}

// Compartment?
/*
 * Copyright (c) 2020 Moddable Tech, Inc
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
	fromBigInt(value: BigInt): ArrayBuffer;
}

interface ArrayBuffer {
	concat(...buffers: ArrayBuffer[]): ArrayBuffer;
}

interface BigIntConstructor {
	bitLength(value: BigInt): number
	fromArrayBuffer(buffer: ArrayBuffer): BigInt
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
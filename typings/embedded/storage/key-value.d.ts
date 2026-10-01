/*
 * Copyright (c) 2024-2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

declare module "embedded:storage/key-value" {
	export interface StorageOpenOptions {
		format?: StorageFormat;
		path: string;
		mode?: "r" | "r+";
	}

	export type StorageFormat = "buffer" | "string" | "uint8" | "int8" | "uint16" | "int16" | "uint32" | "int32" | "uint64" | "int64";

	// the type read() returns for each format
	export interface StorageFormats {
		buffer: ArrayBuffer;
		string: string;
		uint8: number;
		int8: number;
		uint16: number;
		int16: number;
		uint32: number;
		int32: number;
		uint64: bigint;
		int64: bigint;
	}

	/*
		F is the format named in the options passed to open(). Naming it there
		(format: "string") fixes the type read() returns.
		When the options name no format, read() is typed as returning any of the types,
		because the format can be changed later through the format property.
	*/
	export class Storage<F extends StorageFormat = StorageFormat> implements Disposable {
		// Use the default export's open() method to create Storage instances
		close(): void;

		delete(key: string): void;

		read(key: string): StorageFormats[F] | undefined;
		read(key: string, buffer: ByteBuffer): number | undefined;
		write(key: string, value: ByteBuffer | string | number | bigint): void;

		[Symbol.iterator](): Iterator<string>;

		get format(): F;
		set format(value: StorageFormat);
		[Symbol.dispose](): void;
	}

	const KeyValueStorage: {
		open<F extends StorageFormat = StorageFormat>(options: StorageOpenOptions & { format?: F }): Storage<F>;
	};

	export default KeyValueStorage;
}

/*
 * Copyright (c) 2024-2025  Moddable Tech, Inc.
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
		path: string;
		mode?: "r" | "r+";
	}

	export type StorageFormat = "buffer" | "string" | "uint8" | "int8" | "uint16" | "int16" | "uint32" | "int32" | "uint64" | "int64";

	export class Storage {
		// Use the default export's open() method to create Storage instances
		close(): void;

		delete(key: string): void;

		read(key: string): ArrayBuffer | string | number | bigint;
		read(key: string, buffer: BufferLike): number;
		write(key: string, value: BufferLike | string | number | bigint): void;

		[Symbol.iterator](): IterableIterator<string>;

		format: StorageFormat;
	}

	const KeyValueStorage: {
		open(options: StorageOpenOptions): Storage;
	};

	export default KeyValueStorage;
}

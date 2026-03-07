/*
 * Copyright (c) 2025  Moddable Tech, Inc.
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

declare module "embedded:storage/flash" {
	export interface FlashOpenOptions {
		path: string;
		mode?: "r" | "r+";
	}

	export interface FlashStatus {
		size: number;
		blockLength: number;
		blocks: number;
		writeAlign: number;
		eraseValue: number;
	}

	export class Flash {
		// Use the default export's open() method to create Flash instances
		close(): void;

		eraseBlock(start: number, end?: number): void;

		read(byteLength: number, byteOffset: number): ArrayBuffer;
		read(buffer: ByteBuffer, byteOffset: number): number;
		write(buffer: ByteBuffer, byteOffset: number): void;

		status(): FlashStatus;

		format: "buffer";
	}

	const FlashStorage: {
		open(options: FlashOpenOptions): Flash;
		[Symbol.iterator](): IterableIterator<string>;
	};

	export default FlashStorage;
}

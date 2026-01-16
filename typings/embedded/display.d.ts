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

declare module "embedded:display" {
	import type Bitmap from "commodetto/Bitmap";
	import type { Rectangle } from "commodetto/Poco";

	export interface DisplayConstructorOptions {
		port: string;
	}

	export interface DisplayConfigureOptions {
		brightness?: number; // 0-1 range
		rotation?: 0 | 90 | 180 | 270;
		format?: typeof Bitmap.RGB565LE | typeof Bitmap.RGB24 | typeof Bitmap.Gray256;
	}

	export interface DisplayConfiguration {
		brightness: number; // 0-1 range
		rotation: 0 | 90 | 180 | 270;
		format: typeof Bitmap.RGB565LE | typeof Bitmap.RGB24 | typeof Bitmap.Gray256;
	}

	export interface DisplayBeginOptions {
		x: number;
		y: number;
		width: number;
		height: number;
	}

	class Display {
		constructor(options: DisplayConstructorOptions);
		close(): void;

		configure(options: DisplayConfigureOptions): void;
		get configuration(): DisplayConfiguration;

		begin(options: DisplayBeginOptions): void;
		begin(): void;
		send(buffer: ByteBuffer): void;
		send(buffer: ByteBuffer, offset: number, length: number): void;
		end(): void;

		adapInvalid(rectangle: Rectangle): void;

		get width(): number;
		get height(): number;
	}

	export default Display;
}

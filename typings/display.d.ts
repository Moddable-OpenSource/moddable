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
	export interface DisplayConstructorOptions {
		format?: number;
		rotation?: number;
		brightness?: number;
		flip?: "h" | "v" | "hv";
	}

	export interface DisplayConfigureOptions {
		brightness?: number;
		format?: number;
		rotation?: number;
	}

	export interface DisplayConfiguration {
		brightness?: number;
		format?: number;
		rotation?: number;
	}

	export interface DisplayBeginOptions {
		x: number;
		y: number;
		width: number;
		height: number;
		continue?: boolean;
	}

	export interface DisplayRectangle {
		x: number;
		y: number;
		width: number;
		height: number;
	}

	export default class Display {
		constructor(options: DisplayConstructorOptions);
		close(): void;
		configure(options: DisplayConfigureOptions): void;
		get configuration(): DisplayConfiguration;
		begin(options?: DisplayBeginOptions): void;
		send(buffer: BufferLike): void;
		end(): void;
		adapInvalid(options: DisplayRectangle): void;
		get width(): number;
		get height(): number;
	}
}

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

declare module "embedded:storage/files" {
	export class Status {
		size: number;
		mode: number;
		isFile(): boolean;
		isDirectory(): boolean;
		isSymbolicLink(): boolean;
	}

	export class File {
		// Use Directory.openFile() to create File instances
		close(): void;

		read(count: number, position: number): ArrayBuffer;
		read(buffer: BufferLike, position: number): number;
		write(buffer: BufferLike, position: number): void;

		status(): Status;
		setSize(length: number): void;
		flush(): void;
	}

	export interface DirectoryOpenFileOptions {
		path: string;
		mode?: "r" | "r+" | "w" | "w+";
	}

	export interface DirectoryOpenDirectoryOptions {
		path: string;
	}

	export interface DirectoryStatusOptions {
		resolveTarget?: boolean;
	}

	export class Directory {
		// use device.files to access file system root
		close(): void;

		openFile(options: DirectoryOpenFileOptions): File;
		openDirectory(options: DirectoryOpenDirectoryOptions): Directory;

		delete(path: string): boolean;
		move(from: string, to: string, toDirectory?: Directory): void;

		status(path: string, options?: DirectoryStatusOptions): Status;

		createDirectory(path: string): boolean;
		createLink(path: string, target: string): void;
		readLink(path: string): string;

		scan(path?: string): IterableIterator<string>;
		[Symbol.iterator](): IterableIterator<string>;
	}
}

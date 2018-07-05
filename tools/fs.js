/*
 * Copyright (c) 2016-2017  Moddable Tech, Inc.
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
 * This file incorporates work covered by the following copyright and  
 * permission notice:  
 *
 *       Copyright (C) 2010-2016 Marvell International Ltd.
 *       Copyright (C) 2002-2010 Kinoma, Inc.
 *
 *       Licensed under the Apache License, Version 2.0 (the "License");
 *       you may not use this file except in compliance with the License.
 *       You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *       Unless required by applicable law or agreed to in writing, software
 *       distributed under the License is distributed on an "AS IS" BASIS,
 *       WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *       See the License for the specific language governing permissions and
 *       limitations under the License.
 */

export function closeSync(fd) @ "fs_closeSync";
export function copyFileSync(from, to) @ "fs_copyFileSync";
export function deleteDirectory(path) @ "fs_deleteDirectory";
export function deleteFile(path) @ "fs_deleteFile";
export function existsSync(path) @ "fs_existsSync";
export function mkdirSync(path) @ "fs_mkdirSync";
export function readDirSync(path) @ "fs_readDirSync";
export function readFileSync(path) @ "fs_readFileSync";
export function readFileBufferSync(path) @ "fs_readFileBufferSync";
export function openSync(path) @ "fs_openSync";
export function writeFileSync(path, string) @ "fs_writeFileSync";
export function writeFileBufferSync(path, string) @ "fs_writeFileBufferSync";
export function writeSync(fd, string) @ "fs_writeSync";
export function writeBufferSync(fd, it) @ "fs_writeBufferSync";
export function writeByteSync(fd, it) @ "fs_writeByteSync";
export function dumpSync(fd, path) @ "fs_dumpSync";

/*
 * Copyright (c) 2020 Shinya Ishikawa
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

declare module 'pins/smbus' {
  type TypedArray =
    | Int8Array
    | Uint8Array
    | Int16Array
    | Uint16Array
    | Int32Array
    | Uint32Array
    | Uint8ClampedArray
    | Float32Array
    | Float64Array
  class SMBus {
    public constructor(dictionary: { address: number; scl?: number; sda?: number; hz?: number; timeout?: number })
    public constructor(port: number[], pin: number, mode: number)
    public close(): void
    public read(count: number, buffer?: ArrayBuffer): void
    public write(first: any, ...valuesOrStop: (number | string | (number | string)[] | TypedArray | boolean)[]): void
    public readByte(register: number): number
    public readWord(register: number): number
    public readBlock(register: number, count: number, buffer?: ArrayBuffer): Uint8Array
    public writeByte(register: number, value: number): void
    public writeWord(register: number, value: number): void
    public writeBlock(register: number, first: any, ...valuesOrStop: (any | boolean)[]): void
  }
  export { SMBus as default }
}

/*
* Copyright (c) 2022 Shinya Ishikawa
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

declare module "embedded:io/socket/tcp" {
  type TypedArray =
    | Uint8Array
    | Uint8ClampedArray
    | Uint16Array
    | Uint32Array
    | Int8Array
    | Int16Array
    | Int32Array
    | Float32Array
    | Float64Array

  class TCP {
    constructor(options: ({
      address: string;
      port: number;
    } | {
      from: TCP;
    }) & {
      nodelay?: boolean;
      onReadable?: (this: TCP, bytes: number) => void;
      onWritable?: (this: TCP, bytes: number) => void;
      onError?: () => void;
    })
    write(value: number | ArrayBuffer | TypedArray): void;
    read(bytes?: number): number | ArrayBuffer;
    get format(): "number" | "buffer"
    set format(value: "number" | "buffer")
  }

  export default TCP;
}

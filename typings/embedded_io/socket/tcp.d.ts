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
  export type Options = (({
    address: string;
    port: number;
  }) | {
    from: TCP;
  }) & {
    noDelay?: boolean;
    onReadable?: (this: TCP, bytes: number) => void;
    onWritable?: (this: TCP, bytes: number) => void;
    onError?: (this: TCP) => void;
    format?: "number" | "buffer";
    target?: any;
  };
  
  class TCP {
    target?: any;
    constructor(options: Options)
    readonly remoteAddress: string | undefined;
    readonly remotePort: number | undefined;
    read(): number | ArrayBuffer;
    read(byteLength: number): ArrayBuffer;
    read(buffer: ByteBuffer): number;
    write(value: number | ByteBuffer): number;
    close(): void;
    get format(): "number" | "buffer"
    set format(value: "number" | "buffer")
  }
  interface TCP extends Disposable {}
  export default TCP;
}

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
  export type Format = "number" | "buffer";

  export type Options<F extends Format = Format> = (({
    address: string;
    port: number;
  }) | {
    from: TCP;
  }) & {
    noDelay?: boolean;
    onReadable?: (this: TCP<F>, bytes: number) => void;
    onWritable?: (this: TCP<F>, bytes: number) => void;
    onError?: (this: TCP<F>) => void;
    format?: F;
    target?: any;
  };

  /*
    F is the format named in the options when the socket is created. Naming it there
    (format: "buffer") fixes the type read() with no argument returns:
    a byte for "number", an ArrayBuffer for "buffer".
    When the options name no format, read() is typed as returning either, because the
    format can be changed later through the format property.
  */
  interface TCP<F extends Format = Format> extends Disposable {
    target?: any;
    readonly remoteAddress: string | undefined;
    readonly remotePort: number | undefined;
    read(): F extends "number" ? number : ArrayBuffer;
    read(byteLength: number): ArrayBuffer;
    read(buffer: ByteBuffer): number;
    write(value: number | ByteBuffer): number;
    close(): void;
    get format(): F
    set format(value: Format)
    [Symbol.dispose](): void;
  }
  interface TCPConstructor {
    new <F extends Format = Format>(options: Options<F>): TCP<F>;
    readonly prototype: TCP;
  }
  const TCP: TCPConstructor;
  export default TCP;
}

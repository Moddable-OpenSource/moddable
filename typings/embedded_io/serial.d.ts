/*
* Copyright (c) 2022-2025  Shinya Ishikawa
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

declare module "embedded:io/serial" {
  import { PinSpecifier, PortSpecifier, IOOptions } from "embedded:io/_common";
  export type Format = "number" | "buffer";

  export type Options<F extends Format = Format> = IOOptions<"Serial", ({
      receive: PinSpecifier;
      transmit?: PinSpecifier;
    } | {
      receive?: PinSpecifier;
      transmit: PinSpecifier;
    }) & {
      dataTerminalReady?: PinSpecifier;
      requestToSend?: PinSpecifier;
      clearToSend?: PinSpecifier;
      dataSetReady?: PinSpecifier;
      port: PortSpecifier;
    }, {
      baud: number;
      flowControl?: "hardware" | "none";
      onReadable?: (this: Serial<F>, bytes: number) => void;
      onWritable?: (this: Serial<F>, bytes: number) => void;
      format?: F;
      target?: any;
    }>;

  /*
    F is the format named in the options when the serial port is created. Naming it there
    (format: "buffer") fixes the type read() with no argument returns:
    a byte for "number", an ArrayBuffer for "buffer".
    When the options name no format, read() is typed as returning either, because the
    format can be changed later through the format property.
  */
  interface Serial<F extends Format = Format> extends Disposable {
    target?: any;
    readonly resolution: number;
    close(): void;
    read(): (F extends "number" ? number : ArrayBuffer) | undefined;
    read(byteLength: number): ArrayBuffer | undefined;
    read(buffer: ByteBuffer): number | undefined;
    write(value: number | ByteBuffer): void;
    flush(): void;
    flush(input: number, output: number): void;
    set(options: {
      dataTerminalReady?: boolean;
      requestToSend?: boolean;
      break?: boolean;
    }): void
    get format(): F
    set format(value: Format)
    [Symbol.dispose](): void;
  }
  interface SerialConstructor {
    new <F extends Format = Format>(options: Options<F>): Serial<F>;
    readonly prototype: Serial;
  }
  const Serial: SerialConstructor;

  export default Serial;
}

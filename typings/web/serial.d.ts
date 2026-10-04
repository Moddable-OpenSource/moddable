/*
* Copyright (c) 2026 Moddable Tech, Inc.
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

// Web Serial (https://wicg.github.io/serial/): the subset implemented by modules/web/serial
declare module "web/serial" {
  import type { ReadableStream, WritableStream } from "web/streams";

  export interface SerialOptions {
    baudRate: number;
    bufferSize?: number;		// 1 to 16384, default 255
    dataBits?: 8;				// the only value supported
    stopBits?: 1;				// the only value supported
    parity?: "none";			// the only value supported
  }

  export interface SerialOutputSignals {
    dataTerminalReady?: boolean;
    requestToSend?: boolean;
    break?: false;				// break is not supported
  }

  export interface SerialPortInfo {
    usbVendorId?: number;
    usbProductId?: number;
  }

  export interface SerialPort {
    readonly connected: boolean;
    readonly readable: ReadableStream<Uint8Array> | null;		// a byte stream: getReader({ mode: "byob" }) is supported
    readonly writable: WritableStream<ByteBuffer> | null;
    getInfo(): SerialPortInfo;
    open(options: SerialOptions): Promise<void>;
    setSignals(signals: SerialOutputSignals): Promise<void>;
    close(): Promise<void>;
    forget(): Promise<void>;
  }

  export const serial: {
    requestPort(): Promise<SerialPort>;
  };
}

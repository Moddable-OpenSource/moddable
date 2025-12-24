/*
* Copyright (c) 2025 Moddable Tech, Inc.
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

// websocketclient.d.ts

declare module "embedded:network/websocket/client" {
  interface WebSocketClientOptions {
    onReadable?: (this: WebSocketClient, count: number, options: { more: boolean; binary: boolean }) => void;
    onWritable?: (this: WebSocketClient, count: number) => void;
    onControl?: (this: WebSocketClient, opcode: number, data: ArrayBuffer) => void;
    onClose?: (this: WebSocketClient) => void;
    onError?: (this: WebSocketClient) => void;
    format?: "buffer" | "number";
    target?: any;
    attach?: {
      constructor: new (options: any) => any;
    };
    host?: string;
    path?: string;
    port?: number;
    protocol?: string;
    headers?: Map<string, string>;
    dns?: {
      io: new (options: any) => any;
    };
    socket?: {
      io: new (options: any) => any;
    };
  }

  interface WebSocketWriteOptions {
    opcode?: number;
    binary?: boolean;
    more?: boolean;
  }

  class WebSocketClient {
    constructor(options: WebSocketClientOptions);

    close(): void;
    write(data: BufferLike, options?: WebSocketWriteOptions): number;
    read(): number | ArrayBuffer;
    read(count: number): ArrayBuffer;
    read(buffer: BufferLike): number;

    format: "buffer" | "number";
    target?: any;

    static readonly text: 1;
    static readonly binary: 2;
    static readonly close: 8;
    static readonly ping: 9;
    static readonly pong: 10;
  }

  export default WebSocketClient;
}
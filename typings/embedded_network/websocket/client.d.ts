/*
* Copyright (c) 2025-2026 Moddable Tech, Inc.
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
  import type TCP from "embedded:io/socket/tcp";
  import type TLS from "embedded:io/socket/tcp/tls";
  import type { ClientSocket } from "embedded:io/socket/tcp/tls";
  import type { DNSResolverProvider } from "embedded:network/dns/resolver/udp";

  export interface WebSocketClientProvider {
    io: typeof WebSocketClient;
    dns: DNSResolverProvider;
    socket: ClientSocket;
  }

  export type Format = "buffer" | "number";

  export interface WebSocketClientOptions<F extends Format = Format> {
    onReadable?: (this: WebSocketClient<F>, count: number, options: { more: boolean; binary: boolean }) => void;
    onWritable?: (this: WebSocketClient<F>, count: number) => void;
    onControl?: (this: WebSocketClient<F>, opcode: number, data: ArrayBuffer) => void;
    onClose?: (this: WebSocketClient<F>) => void;
    onError?: (this: WebSocketClient<F>) => void;
    format?: F;
    target?: any;
    attach?: TCP;
    host?: string;
    path?: string;
    port?: number;
    protocol?: string;
    headers?: Map<string, string>;
    dns?: DNSResolverProvider;
    socket?: ClientSocket;
  }

  export interface WebSocketWriteOptions {
    opcode?: number;
    binary?: boolean;
    more?: boolean;
  }

  /*
    F is the format named in the options when the client is created. Naming it there
    (format: "buffer") fixes the type read() with no argument returns:
    a byte for "number", an ArrayBuffer for "buffer".
    When the options name no format, read() is typed as returning either, because the
    format can be changed later through the format property.
  */
  interface WebSocketClient<F extends Format = Format> extends Disposable {
    close(): void;
    write(data: ByteBuffer, options?: WebSocketWriteOptions): number;
    read(): F extends "number" ? number : ArrayBuffer;
    read(count: number): ArrayBuffer;
    read(buffer: ByteBuffer): number;

    get format(): F;
    set format(value: Format);
    protocol: string | undefined;
    target?: any;

    [Symbol.dispose](): void;
  }
  interface WebSocketClientConstructor {
    new <F extends Format = Format>(options: WebSocketClientOptions<F>): WebSocketClient<F>;
    readonly prototype: WebSocketClient;

    readonly text: 1;
    readonly binary: 2;
    readonly close: 8;
    readonly ping: 9;
    readonly pong: 10;
  }
  const WebSocketClient: WebSocketClientConstructor;

  export default WebSocketClient;
}

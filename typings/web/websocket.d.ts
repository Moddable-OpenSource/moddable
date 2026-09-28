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

declare module "web/websocket" {
  import Headers from "headers"

  interface WebSocketClientOptions {
    url: string;
    protocol?: string;
    keepalive?: number;
    headers?: Headers | Record<string, string>;
    ws?: any;
    wss?: any;
  }

  interface WebSocketAttachOptions {
    attach: any;
  }

  interface WebSocketCloseEvent {
    code: number;
    reason: string;
    wasClean: boolean;
  }

  interface WebSocketMessageEvent {
    data: string | ArrayBuffer;
  }

  interface WebSocketEvent {
    // Generic event object
  }

  interface WebSocketErrorEvent {
    message?: string;
  }

  type WebSocketEventListener = (event: any) => void;

  class WebSocket {
    constructor(url: string, protocol?: string);
    constructor(options: WebSocketClientOptions);
    constructor(options: WebSocketAttachOptions);

    static readonly CONNECTING: 0;
    static readonly OPEN: 1;
    static readonly CLOSING: 2;
    static readonly CLOSED: 3;

    readonly CONNECTING: 0;
    readonly OPEN: 1;
    readonly CLOSING: 2;
    readonly CLOSED: 3;

    binaryType: "arraybuffer";
    readonly bufferedAmount: number;
    readonly extensions: string;
    readonly protocol: string;
    readonly readyState: number;
    readonly url: string;

    onclose: ((this: WebSocket, event: WebSocketCloseEvent) => void) | null;
    onerror: ((this: WebSocket, event: WebSocketErrorEvent) => void) | null;
    onmessage: ((this: WebSocket, event: WebSocketMessageEvent) => void) | null;
    onopen: ((this: WebSocket, event: WebSocketEvent) => void) | null;

    addEventListener(event: "close", listener: (this: WebSocket, event: WebSocketCloseEvent) => void): void;
    addEventListener(event: "error", listener: (this: WebSocket, event: WebSocketErrorEvent) => void): void;
    addEventListener(event: "message", listener: (this: WebSocket, event: WebSocketMessageEvent) => void): void;
    addEventListener(event: "open", listener: (this: WebSocket, event: WebSocketEvent) => void): void;

    removeEventListener(event: "close" | "error" | "message" | "open", listener: WebSocketEventListener): void;

    close(code?: number, reason?: string): void;
    send(data: string | ArrayBuffer | ArrayBufferView): void;
  }

  export default WebSocket;
}

// the specifier of the module in the Moddable SDK runtime
declare module "WebSocket" {
  export { default } from "web/websocket";
}

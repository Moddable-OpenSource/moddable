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

  interface WebSocketOptions {
    url: string;
    protocol?: string;
    keepalive?: number;
    headers?: Headers | Record<string, string>;
    attach?: any;
    ws?: any;
    wss?: any;
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
    constructor(options: WebSocketOptions);

    binaryType: "arraybuffer";
    readonly bufferedAmount: number;
    readonly extensions: string;
    readonly protocol: string;
    readonly readyState: number;
    readonly url: string;

    onclose?(event: WebSocketCloseEvent): void;
    onerror?(event: WebSocketErrorEvent): void;
    onmessage?(event: WebSocketMessageEvent): void;
    onopen?(event: WebSocketEvent): void;

    addEventListener(event: "close", listener: (event: WebSocketCloseEvent) => void): void;
    addEventListener(event: "error", listener: (event: WebSocketErrorEvent) => void): void;
    addEventListener(event: "message", listener: (event: WebSocketMessageEvent) => void): void;
    addEventListener(event: "open", listener: (event: WebSocketEvent) => void): void;

    removeEventListener(event: "close" | "error" | "message" | "open", listener: WebSocketEventListener): void;

    close(code?: number, reason?: string): void;
    send(data: string | ArrayBuffer | ArrayBufferView): void;
  }

  export default WebSocket;
}
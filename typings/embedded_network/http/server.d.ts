/*
 * Copyright (c) 2026  Moddable Tech, Inc.
 *
 *   This file is part of the Moddable SDK Runtime.
 *
 *   The Moddable SDK Runtime is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU Lesser General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   The Moddable SDK Runtime is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public License
 *   along with the Moddable SDK Runtime.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

declare module "embedded:network/http/server" {
    import type Listener from "embedded:io/socket/listener";
    import type TCP from "embedded:io/socket/tcp";

    export interface HTTPRequest {
        method: string;
        path: string;
        headers: Map<string, string>;
    }

    export interface HTTPResponse {
        status: number;
        headers: Map<string, string | number>;
    }

    export interface HTTPConnectionHandlers {
        onRequest?(this: HTTPConnection, request: HTTPRequest): void;
        onReadable?(this: HTTPConnection, count: number): void;
        onResponse?(this: HTTPConnection, response: HTTPResponse): void;
        onWritable?(this: HTTPConnection, count: number): void;
        onDone?(this: HTTPConnection): void;
        onError?(this: HTTPConnection): void;
    }

    export interface HTTPConnection {
        close(): void;
        detach(): TCP;
        accept(options: HTTPConnectionHandlers): void;
        respond(options: HTTPResponse): void;
        read(count?: number): ArrayBuffer | undefined;
        write(bytes?: ByteBuffer): number;
        route: HTTPConnectionHandlers;
        format: "buffer";
    }

    export interface HTTPServerOptions {
        io: typeof Listener;
        port?: number;
        onConnect(this: HTTPServer, connection: HTTPConnection): void;
    }

    export default class HTTPServer {
        constructor(options: HTTPServerOptions);
        close(): void;
        readonly port: number;
    }
}

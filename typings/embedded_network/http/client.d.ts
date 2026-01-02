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

declare module "embedded:network/http/client" {
  import type { Buffer } from "embedded:io/_common"
  import type { TCPDevice } from "embedded:io/socket/tcp";
  import type { TLSDevice } from "embedded:io/socket/tls";
  import type { DNSUDPDevice } from "embedded:io/socket/dns";

  export interface HTTPClientOptions {
    socket: TCPDevice | TLSDevice;
    port?: number;
    host: string;
    dns: DNSUDPDevice;
    onError?: (err:any)=>void
  }

  export interface RequestOptions {
    method?: string
    path?: string
    headers?: Map<string, string|string[]>
    onHeaders?: (this: HTTPRequest, status:number, headers: Map<String, string>) => void
    onReadable?: (this: HTTPRequest, count: number) => void
    onWritable?: (this: HTTPRequest, count: number) => void
    onDone?: (this: HTTPRequest, error: Error|null) => void // note: error is empty Error object
  }

  export type HTTPClientDevice = HTTPClientOptions & { io: typeof HTTPClient };

  export interface HTTPRequest {
		read(): number;
		read(byteLength: number): ArrayBuffer;
		read(buffer: Buffer): void;
    write(value: Buffer|undefined): void;
  }

  export default class HTTPClient {
    constructor(options: HTTPClientOptions)
    request(options: RequestOptions): HTTPRequest
    close(): void
  }

}

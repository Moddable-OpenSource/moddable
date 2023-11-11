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

  export interface ClientOptions {
    socket: any
    port?:number
    host: string
    dns: any
    onError: (err:any)=>void
  }

  export interface RequestOptions {
    method?: string
    path?: string
    headers?: Map<string, string|string[]>
    onHeaders?: (this: HTTPRequest, status:number, headers:Record<string, string>) => void
    onReadable?: (this: HTTPRequest, count: number) => void
    onWritable?: (this: HTTPRequest, count: number) => void
    onDone?: (this: HTTPRequest, error: Error|null) => void // note: error is empty Error object
  }

  export interface HTTPRequest {
    read(byteLength?: number): ArrayBuffer|undefined;
    read(buffer: Buffer): void;
    write(value: Buffer|undefined): void;
  }

  export default class HTTPClient {
    constructor(options: ClientOptions)
    request(options: RequestOptions): HTTPRequest
    close(): void
  }

}

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

declare module "fetch" {
  import {URL, URLSearchParams} from "url"
  import Headers from "headers"

  class Response {
    get bodyUsed(): boolean;
    get headers(): Headers;
    get ok(): boolean;
    get redirected(): boolean;
    get status(): number;
    get statusText(): string;
    get url(): string;
    arrayBuffer(): Promise<ArrayBuffer>;
    json(): Promise<any>;
    text(): Promise<string>;
  }

  interface RequestInit {
    method?: string;
    headers?: Headers | Record<string, string>;
    body?: string | ByteBuffer | URLSearchParams;
  }

  function fetch(resource: string | URL, options?: RequestInit): Promise<Response>;

  export { fetch };
  export default fetch;
}

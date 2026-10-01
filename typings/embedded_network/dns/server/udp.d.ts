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

declare module "embedded:network/dns/server/udp" {
  import type UDP from "embedded:io/socket/udp";

  export interface Options {
    socket: { io: typeof UDP };
    // returns the address for the name, or a falsy value when the name is not served
    onResolve: (name: string) => string | undefined | null | false;
    ttl?: number;		// seconds, defaults to 60
    port?: number;		// defaults to 53
  }

  class Server implements Disposable {
    constructor(options: Options);
    close(): void;
    [Symbol.dispose](): void;
  }

  export default Server;
}

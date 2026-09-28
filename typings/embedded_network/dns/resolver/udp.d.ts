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

declare module "embedded:network/dns/resolver/udp" {
  import type UDP from "embedded:io/socket/udp";

  export interface DNSResolverProvider {
    io: typeof Resolver
    servers: string[]
    socket: { io: typeof UDP, [option: string]: unknown }
  }

  interface Options {
    host: string
    onResolved?: (this: Resolver, host: string, address: string) => void
    onError?: (this: Resolver, host: string) => void
  }

  class Resolver implements Disposable {
    target?: any
    constructor(options: Record<string, any>)
    close(): void
    resolve(options: Options): void
    [Symbol.dispose](): void
  }

  export default Resolver
}

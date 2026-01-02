/*
* Copyright (c) 2022-2025 Shinya Ishikawa
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

declare module "embedded:io/socket/udp" {
  import type { Buffer } from "embedded:io/_common";

  interface UDPOptionsBase {
    port?: number;
    address?: string;
    onReadable?: (this: UDP, packets: number) => void;
    onError?: () => void;
    format?: "buffer";
  }
  
  interface UDPMulticastOptions {
    multicast: string;
    timeToLive: number;
  }
  
  export type UDPOptions = UDPOptionsBase & ({} | UDPMulticastOptions);

  export type UDPDevice = UDPOptions & { io: typeof UDP };

  class UDP {
    constructor(options: UDPOptions)
    write(buffer: Buffer, address: string, port: number): void;
    read(): ArrayBuffer & {
      address: string;
      port: number;
    };
    get format(): "buffer"
    set format(value: "buffer")
  }
  
  export default UDP;
}

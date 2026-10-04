/*
* Copyright (c) 2019-2025  Bradley Farias
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

declare module "socket" {
  type RawSocketOptions = {
    host?: string,
    address?: string,
    port: number,
    kind: "RAW",
    protocol?: number,
  };
  export type TCPSocketOptions = {
    kind?: "TCP",
    host?: string,
    addresss?: string,
    port?: number,
    noDelay?: boolean,
    keepalive?: {
      idle: number,
      interval: number,
      count: number
    }
  };
  type UDPSocketOptions = {
    host?: string,
    addresss?: string,
    port: number,
    kind: "UDP",
  };
  type WriteData = number | string | ByteBuffer;
  
  type MessageError = -2;
  type MessageDisconnect = -1;
  type MessageConnect = 1;
  type MessageDataReceived = 2;
  type MessageDataSent = 3;
  type MessageStatus = (
    MessageError |
    MessageDisconnect |
    MessageConnect |
    MessageDataReceived |
    MessageDataSent
  );
  /** @deprecated Use the ECMA-419 sockets, embedded:io/socket/tcp, embedded:io/socket/udp and embedded:io/socket/listener */
  abstract class SocketBase {
    close(): void;
    read<T extends typeof String | typeof ArrayBuffer>(
      type: T,
      until?: number
    ): T extends typeof String ? string : ArrayBuffer;
    callback: (message: MessageStatus, value?: number) => void;
  }
  /** @deprecated Use the ECMA-419 sockets, embedded:io/socket/tcp, embedded:io/socket/udp and embedded:io/socket/listener */
  class RawSocket extends SocketBase {
    write(ip: string, data: ArrayBuffer): void;
    write(): number;
  }
  /** @deprecated Use the ECMA-419 sockets, embedded:io/socket/tcp, embedded:io/socket/udp and embedded:io/socket/listener */
  class TCPSocket extends SocketBase {
    write(data: WriteData, ...moreData: WriteData[]): void;
    write(): number;
  }
  /** @deprecated Use the ECMA-419 sockets, embedded:io/socket/tcp, embedded:io/socket/udp and embedded:io/socket/listener */
  class UDPSocket extends SocketBase {
    write(ip: string, port: number, data: ArrayBuffer): void;
    write(): number;
  }
  export type ListenerOptions = {
    port?: number,
    address?: string
  };
  /** @deprecated Use the ECMA-419 sockets, embedded:io/socket/tcp, embedded:io/socket/udp and embedded:io/socket/listener */
  class Listener {
    constructor(options: ListenerOptions);
    callback: (this: Listener) => void;
  }
  type Socket = RawSocket | TCPSocket | UDPSocket;
  /** @deprecated Use the ECMA-419 sockets, embedded:io/socket/tcp, embedded:io/socket/udp and embedded:io/socket/listener */
  var Socket: {
    new <T extends RawSocketOptions | TCPSocketOptions | UDPSocketOptions>(
      dictionary: T
    ): T extends RawSocketOptions ?
      RawSocket : T extends TCPSocketOptions ? TCPSocket : UDPSocket;
    new ({listener: Listener}): TCPSocket;
  };
  export {Socket, Listener};
}

declare module "socket" {
  type RawSocketOptions = {
    host: string,
    port: number,
    kind: "RAW",
    protocol?: number,
  };
  export type TCPSocketOptions = {
    host: string,
    port: number,
    kind?: "TCP",
    noDelay?: boolean,
    keepalive?: {
      idle: number,
      interval: number,
      count: number
    }
  };
  type UDPSocketOptions = {
    host: string,
    port: number,
    kind: "UDP",
  };
  type WriteData = number | string | ArrayBuffer;
  
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
  abstract class SocketBase {
    close(): void;
    read<T extends typeof String | typeof ArrayBuffer>(
      type: T,
      until?: number
    ): T extends typeof String ? string : ArrayBuffer;
    callback: (message: MessageStatus, value?: number) => void;
  }
  class RawSocket extends SocketBase {
    write(ip: string, data: ArrayBuffer);
    write(): number;
  }
  class TCPSocket extends SocketBase {
    write(data: WriteData, ...moreData: WriteData[]);
    write(): number;
  }
  class UDPSocket extends SocketBase {
    write(ip: string, port: number, data: ArrayBuffer);
    write(): number;
  }
  export type ListenerOptions = {port: number};
  class Listener {
    constructor(options: ListenerOptions);
    callback: (this: Listener) => void;
  }
  var Socket: {
    new <T extends RawSocketOptions | TCPSocketOptions | UDPSocketOptions>(
      dictionary: T
    ): T extends RawSocketOptions ?
      RawSocket : T extends TCPSocketOptions ? TCPSocket : UDPSocket;
    new ({listener: Listener}): TCPSocket;
  };
  export {Socket, Listener};
}

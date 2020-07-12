declare module "websocket" {
  class Client {
    constructor(options: import('socket').TCPSocketOptions & {
      port?: number,
      path?: string
    });
    close(): void;
    write(data: string | ArrayBuffer);
    callback: (message: number, value?: any) => void;
  }
  class Server {
    constructor(options: import('socket').ListenerOptions);
    close(): void;
    write(message: string | ArrayBuffer): void;
    callback: (message: number, value?: any) => void;
  }
  export {Client, Server};
}

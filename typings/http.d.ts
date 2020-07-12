declare module "http" {
  type RequestBodyFragment = 0;
  type RequestStatusCode = 1;
  type RequestHeaderReceived = 2;
  type RequestAllHeaders = 3;
  type RequestResponseFragment = 4;
  type RequestAllResponse = 5;
  type RequestStatus = (
    RequestBodyFragment |
    RequestStatusCode |
    RequestHeaderReceived |
    RequestAllHeaders |
    RequestResponseFragment |
    RequestAllResponse
  );
  class Request {
    constructor(options: {
      port?: number,
      path?: string,
      method?: string,
      headers?: string[],
      body?: boolean | string | ArrayBuffer,
      response?: typeof String | typeof ArrayBuffer
    });
    close(): void;
    read<T extends typeof String | typeof ArrayBuffer>(
      type: T,
      until?: number
    ): T extends typeof String ? string : ArrayBuffer;
    // TODO: better
    callback: (message: RequestStatus, val1?: any, val2?: any) => void;
  }
  class Server {
    constructor(
      options: import('socket').TCPSocketOptions & {
        port?: number
      }
    );
    close();
    // TODO: better
    callback: (message: number, val1?: any, val2?: any) => void;
  }
  export {Request, Server};
}
